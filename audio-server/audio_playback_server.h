#pragma once
#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <iostream>
#include <lame/lame.h>
#include <vector>
#include <fstream>
#include <opus/opus.h>


class AudioPlaybackServer {
public:
    AudioPlaybackServer();
    ~AudioPlaybackServer();

    bool Start(uint16_t port);
    void AcceptAndHandle();
    void ConvertAndWriteAudio();

private:
    SOCKET listenSock;
    SOCKET clientSock;

    // Audio playback members
    IMMDeviceEnumerator* pEnumerator;
    IMMDevice* pDevice;
    IAudioClient* pAudioClient;
    IAudioRenderClient* pRenderClient;

    // LAME decoder
    std::vector<short> pcmLeft;
    std::vector<short> pcmRight;
    std::vector<float> pcmInterleaved;
    std::vector<uint8_t> rawOpusAudio;

    bool deviceIsFloat32;
    int deviceSampleRate;
    int deviceChannels;
    OpusDecoder* decoder;

};
