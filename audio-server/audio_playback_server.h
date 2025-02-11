#pragma once
#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <iostream>
#include <lame/lame.h>
#include <vector>
#include <fstream>


class AudioPlaybackServer {
public:
    AudioPlaybackServer();
    ~AudioPlaybackServer();

    bool Start(uint16_t port);
    void AcceptAndHandle();
    void ConvertAndWriteAudio(const std::vector<uint8_t>& mp3Data);

private:
    SOCKET listenSock;
    SOCKET clientSock;

    // Audio playback members
    IMMDeviceEnumerator* pEnumerator;
    IMMDevice* pDevice;
    IAudioClient* pAudioClient;
    IAudioRenderClient* pRenderClient;

    // LAME decoder
    hip_t hipDecoder;
    std::vector<short> pcmLeft;
    std::vector<short> pcmRight;
    std::vector<short> pcmInterleaved;

    bool deviceIsFloat32;
    int deviceSampleRate;
    int deviceChannels;
};
