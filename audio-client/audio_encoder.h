#pragma once
#include <cstdint>
#include <vector>
#include <memory>


class IAudioEncoder {
public:
    virtual ~IAudioEncoder() = default;

    // Initialize encoder with audio format parameters
    virtual bool Initialize(int sampleRate, int channels, int bitsPerSample) = 0;

    // Encode audio data and return encoded bytes
    virtual std::vector<uint8_t> Encode(const float* data, size_t frameCount) = 0;

    // Flush any remaining data and finish encoding
    virtual std::vector<uint8_t> Finalize() = 0;

};
