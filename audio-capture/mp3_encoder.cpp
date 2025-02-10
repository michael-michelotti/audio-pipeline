#include "mp3_encoder.h"


MP3Encoder::MP3Encoder(int bitrate) : bitrate(bitrate), lameFlags(nullptr) {
    mp3Buffer.resize(1024 * 1024); // 1MB buffer for encoded data
}

MP3Encoder::~MP3Encoder() {
    if (lameFlags) {
        lame_close(lameFlags);
    }
}

bool MP3Encoder::Initialize(int sampleRate, int channels, int bitsPerSample) {
    lameFlags = lame_init();
    if (!lameFlags) return false;

    lame_set_in_samplerate(lameFlags, sampleRate);
    lame_set_num_channels(lameFlags, channels);
    lame_set_brate(lameFlags, bitrate);
    lame_set_quality(lameFlags, 2); // High quality
    lame_set_VBR(lameFlags, vbr_off);

    if (lame_init_params(lameFlags) < 0) {
        lame_close(lameFlags);
        lameFlags = nullptr;
        return false;
    }

    return true;
}

std::vector<uint8_t> MP3Encoder::Encode(const float* data, size_t frameCount) {
    if (!lameFlags) return {};

    const float* leftChannel = data;
    const float* rightChannel = data + 1;

    int encodedBytes = lame_encode_buffer_interleaved_ieee_float(
        lameFlags,
        const_cast<float*>(data),
        frameCount,
        mp3Buffer.data(),
        mp3Buffer.size()
    );

    if (encodedBytes < 0) return {};

    return std::vector<uint8_t>(mp3Buffer.begin(), mp3Buffer.begin() + encodedBytes);
}

std::vector<uint8_t> MP3Encoder::Finalize() {
    if (!lameFlags) return {};

    int encodedBytes = lame_encode_flush(
        lameFlags,
        mp3Buffer.data(),
        mp3Buffer.size()
    );

    if (encodedBytes < 0) return {};

    return std::vector<uint8_t>(mp3Buffer.begin(), mp3Buffer.begin() + encodedBytes);
}
