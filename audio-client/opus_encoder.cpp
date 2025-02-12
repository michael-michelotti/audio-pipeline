#pragma once
#define NOMINMAX
#include <opus/opus.h>
#include <fstream>
#include <iostream>
#include "audio_encoder.h"
#include "audio_network.h"

#include "opus_encoder.h"


OpusEncoder::OpusEncoder(int bitrate) : 
	encoder(nullptr),
	bitrate(bitrate),
	frameSize(960),
	bufferPos(0),
    oggSerialNo(rand()),
    packetNo(0),
    granulePos(0) {

	encoderBuffer.resize(32768);
	inputBuffer.resize(frameSize * 2);
    ogg_stream_init(&oggStream, oggSerialNo);
}

std::vector<uint8_t> OpusEncoder::GetHeaderData() {
    std::vector<uint8_t> headerData;
    ogg_page oggPage;
    while (ogg_stream_flush(&oggStream, &oggPage) > 0) {
        headerData.insert(headerData.end(),
            oggPage.header,
            oggPage.header + oggPage.header_len);
        headerData.insert(headerData.end(),
            oggPage.body,
            oggPage.body + oggPage.body_len);
    }
    return headerData;
}


OpusEncoder::~OpusEncoder() {
	if (encoder) {
		opus_encoder_destroy(encoder);
	}
    ogg_stream_clear(&oggStream);
}

bool OpusEncoder::Initialize(int sampleRate, int channels, int bitsPerSample) {
    if (sampleRate != 48000 || channels != 2) {
        std::cerr << "Opus encoder requires 48kHz stereo input" << std::endl;
        return false;
    }

    int error;
    encoder = opus_encoder_create(48000, channels, OPUS_APPLICATION_AUDIO, &error);
    if (error != OPUS_OK || !encoder) {
        std::cerr << "Failed to create Opus encoder: " << opus_strerror(error) << std::endl;
        return false;
    }

    // Opus encoder settings
    opus_encoder_ctl(encoder, OPUS_SET_BITRATE(bitrate * 1000));
    opus_encoder_ctl(encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_MUSIC));
    opus_encoder_ctl(encoder, OPUS_SET_COMPLEXITY(10));

    // --- OpusHead Packet ---
    unsigned char header[19] = {
        'O', 'p', 'u', 's', 'H', 'e', 'a', 'd',   // Magic Signature
        1,                                        // Version
        2,                                        // Channel Count
        0x38, 0x01,                               // Pre-skip (312 samples in little-endian)
        0x80, 0xBB, 0x00, 0x00,                   // Original sample rate (48000 Hz in little-endian)
        0x00, 0x00,                               // Output gain (0)
        0                                         // Channel Mapping Family (0 = stereo)
    };

    ogg_packet oggHeader;
    oggHeader.packet = header;
    oggHeader.bytes = sizeof(header);
    oggHeader.b_o_s = 1;    // This is the beginning of the stream
    oggHeader.e_o_s = 0;
    oggHeader.granulepos = 0;
    oggHeader.packetno = packetNo++;

    if (ogg_stream_packetin(&oggStream, &oggHeader) != 0) {
        std::cerr << "Failed to insert OpusHead packet into Ogg stream" << std::endl;
    }

    // --- Flush Immediately ---
    ogg_page oggPage;
    while (ogg_stream_flush(&oggStream, &oggPage) > 0) {
        encoderBuffer.insert(encoderBuffer.end(),
            oggPage.header,
            oggPage.header + oggPage.header_len);
        encoderBuffer.insert(encoderBuffer.end(),
            oggPage.body,
            oggPage.body + oggPage.body_len);
    }

    // --- OpusTags Packet ---
    const char* vendor = "opus encoder";
    uint32_t vendorLength = strlen(vendor);

    std::vector<unsigned char> tags = {
        'O', 'p', 'u', 's', 'T', 'a', 'g', 's'
    };

    // Vendor string length (little-endian)
    tags.push_back(vendorLength & 0xFF);
    tags.push_back((vendorLength >> 8) & 0xFF);
    tags.push_back((vendorLength >> 16) & 0xFF);
    tags.push_back((vendorLength >> 24) & 0xFF);

    // Vendor string
    tags.insert(tags.end(), vendor, vendor + vendorLength);

    // User comment list length = 0 (no additional tags)
    tags.push_back(0); tags.push_back(0); tags.push_back(0); tags.push_back(0);

    ogg_packet oggTags;
    oggTags.packet = tags.data();
    oggTags.bytes = tags.size();
    oggTags.b_o_s = 0;
    oggTags.e_o_s = 0;
    oggTags.granulepos = 0;
    oggTags.packetno = packetNo++;

    if (ogg_stream_packetin(&oggStream, &oggTags) != 0) {
        std::cerr << "Failed to insert OpusTags packet into Ogg stream" << std::endl;
    }

    // --- Flush Tags Immediately ---
    while (ogg_stream_flush(&oggStream, &oggPage) > 0) {
        encoderBuffer.insert(encoderBuffer.end(),
            oggPage.header,
            oggPage.header + oggPage.header_len);
        encoderBuffer.insert(encoderBuffer.end(),
            oggPage.body,
            oggPage.body + oggPage.body_len);
    }

    return true;
}


std::vector<uint8_t> OpusEncoder::Encode(const float* data, size_t frameCount) {
    std::vector<uint8_t> result;
    size_t pos = 0;

    while (pos < frameCount) {
        size_t samplesToCopy = std::min(frameCount - pos,
            frameSize - bufferPos);

        memcpy(inputBuffer.data() + (bufferPos * 2),
            data + (pos * 2),
            samplesToCopy * 2 * sizeof(float));

        bufferPos += samplesToCopy;
        pos += samplesToCopy;

        if (bufferPos == frameSize) {
            opus_int32 encodedBytes = opus_encode_float(
                encoder,
                inputBuffer.data(),
                frameSize,
                encoderBuffer.data(),
                encoderBuffer.size()
            );

            if (encodedBytes > 0) {
                ogg_packet oggPacket;
                oggPacket.packet = encoderBuffer.data();
                oggPacket.bytes = encodedBytes;
                oggPacket.b_o_s = 0;
                oggPacket.e_o_s = 0;
                oggPacket.granulepos = granulePos;
                oggPacket.packetno = packetNo++;

                granulePos += samplesToCopy;

                ogg_stream_packetin(&oggStream, &oggPacket);

                ogg_page oggPage;
                while (ogg_stream_pageout(&oggStream, &oggPage) > 0) {
                    result.insert(result.end(),
                        oggPage.header,
                        oggPage.header + oggPage.header_len);
                    result.insert(result.end(),
                        oggPage.body,
                        oggPage.body + oggPage.body_len);
                }
            }  // Missing this closing brace for if (encodedBytes > 0)

            bufferPos = 0;
        }
    }
    return result;
}

std::vector<uint8_t> OpusEncoder::Finalize() {
    std::vector<uint8_t> result;

    if (bufferPos > 0) {
        std::fill(inputBuffer.begin() + (bufferPos * 2),
            inputBuffer.end(),
            0.0f);

        opus_int32 encodedBytes = opus_encode_float(
            encoder,
            inputBuffer.data(),
            frameSize,
            encoderBuffer.data(),
            encoderBuffer.size()
        );

        if (encodedBytes > 0) {
            ogg_packet oggPacket;
            oggPacket.packet = encoderBuffer.data();
            oggPacket.bytes = encodedBytes;
            oggPacket.b_o_s = 0;
            oggPacket.e_o_s = 1;  // End of stream
            oggPacket.granulepos = granulePos;
            oggPacket.packetno = packetNo++;

            ogg_stream_packetin(&oggStream, &oggPacket);

            ogg_page oggPage;
            while (ogg_stream_flush(&oggStream, &oggPage) > 0) {
                result.insert(result.end(),
                    oggPage.header,
                    oggPage.header + oggPage.header_len);
                result.insert(result.end(),
                    oggPage.body,
                    oggPage.body + oggPage.body_len);
            }
        }
    }

    ogg_page finalPage;
    while (ogg_stream_flush(&oggStream, &finalPage) > 0) {
        result.insert(result.end(),
            finalPage.header,
            finalPage.header + finalPage.header_len);
        result.insert(result.end(),
            finalPage.body,
            finalPage.body + finalPage.body_len);
    }

    return result;
}

NetworkOpusEncoder::NetworkOpusEncoder(int bitrate) : OpusEncoder(bitrate) {
    if (!sender.Connect("127.0.0.1", 12345)) {
        throw std::runtime_error("Failed to connect to server");
    }
}

std::vector<uint8_t> NetworkOpusEncoder::Encode(const float* data, size_t frameCount) {
    auto encoded = OpusEncoder::Encode(data, frameCount);
    if (!encoded.empty()) {
        uint32_t size = static_cast<uint32_t>(encoded.size());
        sender.SendData(reinterpret_cast<const char*>(&size), sizeof(size));
        sender.SendData(reinterpret_cast<const char*>(encoded.data()), encoded.size());
    }
    return encoded;
}
