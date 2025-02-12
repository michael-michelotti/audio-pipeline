#define NOMINMAX
#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <functiondiscoverykeys_devpkey.h>
#include <thread>
#include <chrono>

#include <opus/opus.h>

#include "audio_playback_server.h"


void CheckAudioFormat(WAVEFORMATEX* pwfx) {
    std::cout << "\nWASAPI Audio Format:" << std::endl;
    std::cout << "Sample Rate: " << pwfx->nSamplesPerSec << std::endl;
    std::cout << "Channels: " << pwfx->nChannels << std::endl;
    std::cout << "Bits Per Sample: " << pwfx->wBitsPerSample << std::endl;
    std::cout << "Block Align: " << pwfx->nBlockAlign << std::endl;
    std::cout << "Format Tag: " << pwfx->wFormatTag << std::endl;
}

AudioPlaybackServer::AudioPlaybackServer() : listenSock(INVALID_SOCKET), clientSock(INVALID_SOCKET),
    pEnumerator(nullptr), pDevice(nullptr), pAudioClient(nullptr),
    pRenderClient(nullptr), deviceIsFloat32(false), deviceSampleRate(0), deviceChannels(0) {

    int error;
    decoder = opus_decoder_create(48000, 2, &error);

    pcmInterleaved.resize(480 * 2);
    rawOpusAudio.resize(1024);

    // Initialize COM and Winsock
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to initialize COM");
    }

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        CoUninitialize();
        throw std::runtime_error("WSAStartup failed");
    }

    // Initialize audio playback
    hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        nullptr,
        CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator),
        (void**)&pEnumerator
    );
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create device enumerator");
    }

    // Get default audio output device
    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to get default audio endpoint");
    }

    // Activate audio client
    hr = pDevice->Activate(
        __uuidof(IAudioClient),
        CLSCTX_ALL,
        nullptr,
        (void**)&pAudioClient
    );
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to activate audio client");
    }

    // Get the mix format
    WAVEFORMATEX* pwfx;
    hr = pAudioClient->GetMixFormat(&pwfx);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to get mix format");
    }

    CheckAudioFormat(pwfx);

    // Initialize audio client
    hr = pAudioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        0,
        10000000, // 1-second buffer
        0,
        pwfx,
        nullptr
    );

    CoTaskMemFree(pwfx);

    if (FAILED(hr)) {
        throw std::runtime_error("Failed to initialize audio client");
    }

    // Get the render client
    hr = pAudioClient->GetService(
        __uuidof(IAudioRenderClient),
        (void**)&pRenderClient
    );
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to get render client");
    }
}

AudioPlaybackServer::~AudioPlaybackServer() {
    if (clientSock != INVALID_SOCKET) closesocket(clientSock);
    if (listenSock != INVALID_SOCKET) closesocket(listenSock);
    WSACleanup();

    if (pRenderClient) pRenderClient->Release();
    if (pAudioClient) pAudioClient->Release();
    if (pDevice) pDevice->Release();
    if (pEnumerator) pEnumerator->Release();

    CoUninitialize();
}

void AudioPlaybackServer::ConvertAndWriteAudio() {
    HRESULT hr;

    int samples = opus_decode_float(
        decoder,
        rawOpusAudio.data(),
        rawOpusAudio.size(),
        pcmInterleaved.data(),
        480,
        0
    );

    if (samples <= 0) {
        std::cerr << "No samples decoded" << std::endl;
    }
    std::cout << "Decoded " << samples << " samples. Sending to output" << std::endl;

    // Get ready state of audio client
    UINT32 numFramesPadding;
    hr = pAudioClient->GetCurrentPadding(&numFramesPadding);
    if (FAILED(hr)) {
        std::cerr << "Failed to get padding" << std::endl;
        return;
    }

    // Get available frames
    UINT32 bufferFrameCount;
    pAudioClient->GetBufferSize(&bufferFrameCount);
    UINT32 numFramesAvailable = bufferFrameCount - numFramesPadding;

    std::cout << "Buffer state: " << numFramesAvailable << " frames available, "
        << numFramesPadding << " frames padding" << std::endl;

    // Only proceed if we have enough space
    if (numFramesAvailable < static_cast<UINT32>(samples)) {
        std::cout << "Buffer too full, waiting..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return;
    }

    BYTE* pData;
    hr = pRenderClient->GetBuffer(samples, &pData);  // Request exactly what we decoded
    if (FAILED(hr)) {
        std::cerr << "Failed to get buffer with hr: " << hr << std::endl;
        return;
    }

    memcpy(pData, pcmInterleaved.data(), samples * deviceChannels * sizeof(float));


    hr = pRenderClient->ReleaseBuffer(samples, 0);
    if (FAILED(hr)) {
        std::cerr << "Failed to release buffer" << std::endl;
    }

    std::cout << "Successfully wrote " << samples << " frames to audio device" << std::endl;
}

bool AudioPlaybackServer::Start(uint16_t port) {
    listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET) return false;

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(listenSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        return false;
    }

    if (listen(listenSock, 1) == SOCKET_ERROR) {
        return false;
    }

    HRESULT hr = pAudioClient->Start();
    if (FAILED(hr)) {
        std::cerr << "Failed to start audio client: " << hr << std::endl;
        return false;
    }
    std::cout << "Audio client successfully started" << std::endl;

    return true;
}

void AudioPlaybackServer::AcceptAndHandle() {
    std::cout << "Waiting for connection..." << std::endl;

    clientSock = accept(listenSock, nullptr, nullptr);
    if (clientSock == INVALID_SOCKET) {
        return;
    }

    std::cout << "Client connected. Receiving and playing audio..." << std::endl;

    while (true) {
        // Receive size of incoming data
        uint32_t dataSize;
        if (recv(clientSock, (char*)&dataSize, sizeof(dataSize), 0) != sizeof(dataSize)) {
            break;
        }

        // Receive the actual data
        size_t totalReceived = 0;
        while (totalReceived < dataSize) {
            int received = recv(clientSock, (char*)rawOpusAudio.data() + totalReceived,
                dataSize - totalReceived, 0);
            if (received <= 0) {
                return;
            }
            totalReceived += received;
        }

        std::cout << "Received and playing chunk of size: " << dataSize << " bytes\n";
        ConvertAndWriteAudio();
    }
}
