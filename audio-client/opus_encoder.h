#pragma once
#include <opus/opus.h>
#include <ogg/ogg.h>
#include <fstream>
#include "audio_encoder.h"
#include "audio_network.h"

// Simple Opus header structure
struct OpusHeader {
    uint8_t version;
    uint8_t channels;
    uint16_t pre_skip;
    uint32_t input_sample_rate;
    int16_t gain;
    uint8_t channel_mapping;
    uint8_t nb_streams;
    uint8_t nb_coupled;
    uint8_t stream_map[255];
};

class OpusEncoder : public IAudioEncoder {
public:
    OpusEncoder(int bitrate = 128);
    ~OpusEncoder() override;

    bool Initialize(int sampleRate, int channels, int bitsPerSample) override;
    std::vector<uint8_t> Encode(const float* data, size_t frameCount) override;
    std::vector<uint8_t> Finalize() override;
    std::vector<uint8_t> GetHeaderData();

    std::vector<uint8_t> encoderBuffer;
    ogg_stream_state oggStream;


private:
    OpusEncoder* encoder;
    int bitrate;
    const size_t frameSize;

    std::vector<float> inputBuffer;
    size_t bufferPos;

    // Ogg stream state
    std::vector<uint8_t> headerData;
    int oggSerialNo;
    ogg_int64_t packetNo;
    ogg_int64_t granulePos;
};


class NetworkOpusEncoder : public OpusEncoder {
public:
    NetworkOpusEncoder(int bitrate = 128);
    std::vector<uint8_t> Encode(const float* data, size_t frameCount) override;

private:
    AudioSender sender;
};
