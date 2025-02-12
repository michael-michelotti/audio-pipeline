#pragma once
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <vector>
#include <opus/opus.h>
#include <mutex>
#include <queue>

#pragma comment(lib, "avrt.lib")


class OpusPlaybackServer {
public:
    OpusPlaybackServer();
    ~OpusPlaybackServer();

    bool Start(uint16_t port);
    void Stop();
    void AcceptAndHandle();
    void ConvertAndWriteAudio();

private:
    SOCKET listenSock;
    SOCKET clientSock;

    // COM members
    IMMDeviceEnumerator* pEnumerator;
    IMMDevice* pDevice;
    IAudioClient* pAudioClient;
    IAudioRenderClient* pRenderClient;
    HANDLE hStopEvent;
    HANDLE hCaptureEvent;
    HANDLE hCaptureThread;

    // Audio data buffers
    std::vector<float> pcmInterleaved;
    std::mutex audioMutex;
    std::queue<std::vector<uint8_t>> audioQueue;

    OpusDecoder* decoder;
};
