#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <functiondiscoverykeys_devpkey.h>

#include "audio_playback_server.h"


AudioPlaybackServer::AudioPlaybackServer() : listenSock(INVALID_SOCKET), clientSock(INVALID_SOCKET),
    pEnumerator(nullptr), pDevice(nullptr), pAudioClient(nullptr),
    pRenderClient(nullptr) {

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
        std::vector<uint8_t> buffer(dataSize);
        size_t totalReceived = 0;
        while (totalReceived < dataSize) {
            int received = recv(clientSock, (char*)buffer.data() + totalReceived,
                dataSize - totalReceived, 0);
            if (received <= 0) {
                return;
            }
            totalReceived += received;
        }

        // Get ready state of audio client
        UINT32 numFramesPadding;
        pAudioClient->GetCurrentPadding(&numFramesPadding);

        // Get available frames
        UINT32 bufferFrameCount;
        pAudioClient->GetBufferSize(&bufferFrameCount);
        UINT32 numFramesAvailable = bufferFrameCount - numFramesPadding;;
        BYTE* pData;

        HRESULT hr = pRenderClient->GetBuffer(numFramesAvailable, &pData);
        if (SUCCEEDED(hr)) {
            // Copy received data to audio buffer
            memcpy(pData, buffer.data(), buffer.size());

            hr = pRenderClient->ReleaseBuffer(numFramesAvailable, 0);
            if (FAILED(hr)) {
                std::cerr << "Failed to release buffer" << std::endl;
            }
        }

        std::cout << "Received and playing chunk of size: " << dataSize << " bytes\n";
    }
}
