#pragma once
#include "audio_encoder.h"
#include <lame/lame.h>


class MP3Encoder : public IAudioEncoder {
public:
    MP3Encoder(int bitrate = 320);
    ~MP3Encoder() override;

    bool Initialize(int sampleRate, int channels, int bitsPerSample) override;
    std::vector<uint8_t> Encode(const float* data, size_t frameCount) override;
    std::vector<uint8_t> Finalize() override;

private:
    lame_global_flags* lameFlags;
    int bitrate;
    std::vector<uint8_t> mp3Buffer;
};
