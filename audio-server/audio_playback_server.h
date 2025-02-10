#pragma once
#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <iostream>


class AudioPlaybackServer {
public:
    AudioPlaybackServer();
    ~AudioPlaybackServer();

    bool Start(uint16_t port);
    void AcceptAndHandle();

private:
    SOCKET listenSock;
    SOCKET clientSock;

    // Audio playback members
    IMMDeviceEnumerator* pEnumerator;
    IMMDevice* pDevice;
    IAudioClient* pAudioClient;
    IAudioRenderClient* pRenderClient;
};