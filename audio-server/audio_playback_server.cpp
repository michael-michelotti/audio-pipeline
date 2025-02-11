#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <functiondiscoverykeys_devpkey.h>

#include "audio_playback_server.h"


void WriteWavHeader(std::ofstream& file, uint32_t dataSize) {
    // RIFF chunk
    file.write("RIFF", 4);
    uint32_t fileSize = dataSize + 36; // Size of entire file - 8
    file.write(reinterpret_cast<const char*>(&fileSize), 4);
    file.write("WAVE", 4);

    // Format chunk
    file.write("fmt ", 4);
    uint32_t fmtSize = 16;
    file.write(reinterpret_cast<const char*>(&fmtSize), 4);

    // WAVE_FORMAT_PCM = 1
    uint16_t audioFormat = 1;
    uint16_t numChannels = 2; // Stereo
    uint32_t sampleRate = 44100; // Standard MP3 sample rate
    uint16_t bitsPerSample = 16; // HIP decoder outputs 16-bit PCM
    uint16_t blockAlign = numChannels * (bitsPerSample / 8);
    uint32_t byteRate = sampleRate * blockAlign;

    file.write(reinterpret_cast<const char*>(&audioFormat), 2);
    file.write(reinterpret_cast<const char*>(&numChannels), 2);
    file.write(reinterpret_cast<const char*>(&sampleRate), 4);
    file.write(reinterpret_cast<const char*>(&byteRate), 4);
    file.write(reinterpret_cast<const char*>(&blockAlign), 2);
    file.write(reinterpret_cast<const char*>(&bitsPerSample), 2);

    // Data chunk
    file.write("data", 4);
    file.write(reinterpret_cast<const char*>(&dataSize), 4);
}

void CheckAudioFormat(WAVEFORMATEX* pwfx) {
    std::cout << "\nWASAPI Audio Format:" << std::endl;
    std::cout << "Sample Rate: " << pwfx->nSamplesPerSec << std::endl;
    std::cout << "Channels: " << pwfx->nChannels << std::endl;
    std::cout << "Bits Per Sample: " << pwfx->wBitsPerSample << std::endl;
    std::cout << "Block Align: " << pwfx->nBlockAlign << std::endl;
    std::cout << "Format Tag: " << pwfx->wFormatTag << std::endl;

    if (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        WAVEFORMATEXTENSIBLE* pwfxex = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(pwfx);
        if (pwfxex->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) {
            std::cout << "Format is IEEE FLOAT" << std::endl;
            // We're sending 16-bit PCM but WASAPI expects float!
            std::cout << "WARNING: Format mismatch - we're sending 16-bit PCM to a float device" << std::endl;
        }
        else if (pwfxex->SubFormat == KSDATAFORMAT_SUBTYPE_PCM) {
            std::cout << "Format is PCM" << std::endl;
        }
    }
}

AudioPlaybackServer::AudioPlaybackServer() : listenSock(INVALID_SOCKET), clientSock(INVALID_SOCKET),
    pEnumerator(nullptr), pDevice(nullptr), pAudioClient(nullptr),
    pRenderClient(nullptr), deviceIsFloat32(false), deviceSampleRate(0), deviceChannels(0) {

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

    // Initialize MP3 decoder and buffers
    hipDecoder = hip_decode_init();
    if (!hipDecoder) {
        throw std::runtime_error("Failed to initialize LAME HIP decoder");
    }

    pcmLeft.resize(2304);
    pcmRight.resize(2304);
    pcmInterleaved.resize(2304 * 2);

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

    // After getting pDevice in your initialization:
    IPropertyStore* pProps = nullptr;
    hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
    if (SUCCEEDED(hr)) {
        PROPVARIANT varName;
        PropVariantInit(&varName);

        hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
        if (SUCCEEDED(hr)) {
            std::wcout << L"Audio Device: " << varName.pwszVal << std::endl;
            PropVariantClear(&varName);
        }

        pProps->Release();
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

void AudioPlaybackServer::ConvertAndWriteAudio(const std::vector<uint8_t>& mp3Data) {
    mp3data_struct mp3data;
    int samples = hip_decode(hipDecoder,
        const_cast<unsigned char*>(mp3Data.data()),
        mp3Data.size(),
        pcmLeft.data(),
        pcmRight.data());

    //std::cout << "Decoded " << samples << " samples from " << mp3Data.size() << " bytes of MP3 data" << std::endl;

    //// Check the first few samples to make sure we're getting non-zero audio
    //if (samples > 0) {
    //    std::cout << "First few samples (L,R): ";
    //    int loops = samples < 5 ? samples : 5;
    //    for (int i = 0; i < loops; i++) {
    //        std::cout << "(" << pcmLeft[i] << "," << pcmRight[i] << ") ";
    //    }
    //    std::cout << std::endl;
    //}

    if (samples <= 0) {
        std::cerr << "No samples decoded" << std::endl;
    }

    // Make sure we have enough space in our interleaved buffer
    if (static_cast<size_t>(samples * 2) > pcmInterleaved.size()) {
        std::cerr << "Resizing interleaved buffer from " << pcmInterleaved.size()
            << " to " << samples * 2 << std::endl;
        pcmInterleaved.resize(samples * 2);
    }

    for (int i = 0; i < samples; i++) {
        pcmInterleaved[i * 2] = pcmLeft[i];
        pcmInterleaved[i * 2 + 1] = pcmRight[i];
    }

    // Get ready state of audio client
    UINT32 numFramesPadding;
    pAudioClient->GetCurrentPadding(&numFramesPadding);

    // Get available frames
    UINT32 bufferFrameCount;
    pAudioClient->GetBufferSize(&bufferFrameCount);
    UINT32 numFramesAvailable = bufferFrameCount - numFramesPadding;

    BYTE* pData;
    HRESULT hr = pRenderClient->GetBuffer(numFramesAvailable, &pData);

    //std::cout << "Available frames: " << numFramesAvailable
    //    << " (buffer: " << bufferFrameCount
    //    << ", padding: " << numFramesPadding << ")" << std::endl;

    if (SUCCEEDED(hr)) {
        UINT32 framesToWrite = min(numFramesAvailable,
            static_cast<UINT32>(samples));
        //std::cout << "Writing " << framesToWrite << " frames" << std::endl;

        // Double check our buffer sizes
        if (framesToWrite * 2 * sizeof(short) > pcmInterleaved.size() * sizeof(short)) {
            std::cerr << "Buffer size mismatch!" << std::endl;
            framesToWrite = pcmInterleaved.size() / 2;  // Adjust to prevent overflow
        }

        memcpy(pData, pcmInterleaved.data(), framesToWrite * 2 * sizeof(short));

        hr = pRenderClient->ReleaseBuffer(framesToWrite, 0);
        if (FAILED(hr)) {
            std::cerr << "Failed to release buffer" << std::endl;
        }
    }
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

    // Start audio playback
    HRESULT hr = pAudioClient->Start();
    if (FAILED(hr)) {
        return false;
    }

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
        std::vector<uint8_t> mp3Buffer(dataSize);
        size_t totalReceived = 0;
        while (totalReceived < dataSize) {
            int received = recv(clientSock, (char*)mp3Buffer.data() + totalReceived,
                dataSize - totalReceived, 0);
            if (received <= 0) {
                return;
            }
            totalReceived += received;
        }

        ConvertAndWriteAudio(mp3Buffer);
        std::cout << "Received and playing chunk of size: " << dataSize << " bytes\n";
    }
}
