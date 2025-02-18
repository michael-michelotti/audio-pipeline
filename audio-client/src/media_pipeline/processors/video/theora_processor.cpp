#include <stdexcept>
#include <iostream>

#include <theora/theoraenc.h>

#include "media_pipeline/processors/video/theora_processor.h"
#include "media_pipeline/core/interfaces/i_media_processor.h"
#include "media_pipeline/core/interfaces/i_media_component.h"
#include "media_pipeline/core/media_data.h"

namespace media_pipeline::processors::video {
    using core::MediaData;
    using core::VideoFormat;

    TheoraProcessor::TheoraProcessor() : isInitialized(false), enc_state(nullptr) {}

    TheoraProcessor::~TheoraProcessor() {
        if (enc_state) {
            th_encode_free(enc_state);
        }
    }

    void TheoraProcessor::Start() {
        isInitialized = false;
    }

    void TheoraProcessor::Stop() {
        if (enc_state) {
            th_encode_free(enc_state);
            enc_state = nullptr;
        }
        isInitialized = false;
    }

    MediaData TheoraProcessor::ProcessMediaData(const MediaData& input) {
        if (!isInitialized) {
            InitializeEncoder(input);

            // First frame, write Theora headers
            th_comment comment;
            th_comment_init(&comment);

            std::vector<uint8_t> headerData;
            ogg_packet packet;

            // Get all three header packets
            while (th_encode_flushheader(enc_state, &comment, &packet) > 0) {
                // Store size of this packet
                size_t packetSize = packet.bytes;
                size_t currentSize = headerData.size();
                headerData.resize(currentSize + sizeof(size_t) + packetSize);

                // Write size
                std::memcpy(headerData.data() + currentSize, &packetSize, sizeof(size_t));

                // Write packet data
                std::memcpy(headerData.data() + currentSize + sizeof(size_t),
                    packet.packet,
                    packet.bytes);
            }

            th_comment_clear(&comment);

            // Create and emit header MediaData
            VideoFormat headerFormat = std::get<VideoFormat>(input.format);
            headerFormat.format = VideoFormat::PixelFormat::THEORA_HEADERS;
            MediaData headerOutput = MediaData::createVideo(std::move(headerData), headerFormat);

            return headerOutput;
        }

        const VideoFormat& format = std::get<VideoFormat>(input.format);

        // Convert YUY2 to YUV420P format that Theora expects
        std::vector<uint8_t> yuvData = input.data;

        // Set up Theora picture
        th_ycbcr_buffer ycbcr;
        SetupTheoraPicture(ycbcr, yuvData.data(), format.width, format.height);

        // Encode frame
        if (th_encode_ycbcr_in(enc_state, ycbcr) != 0) {
            throw std::runtime_error("Failed to submit frame to encoder");
        }

        // Get compressed data
        ogg_packet packet;
        std::vector<uint8_t> compressedData;

        if (th_encode_packetout(enc_state, 0, &packet) == 1) {
            compressedData.resize(packet.bytes);
            std::memcpy(compressedData.data(), packet.packet, packet.bytes);
        }

        double timestamp = th_granule_time(enc_state, packet.granulepos);
        std::cout << "timestamp of video packet: " << timestamp << std::endl;

        // Create output MediaData
        VideoFormat outputFormat;
        outputFormat.width = format.width;
        outputFormat.height = format.height;
        outputFormat.frameRate = format.frameRate;
        outputFormat.format = VideoFormat::PixelFormat::THEORA;
        outputFormat.isKeyFrame = th_packet_iskeyframe(&packet) > 0;
        outputFormat.granulepos = packet.granulepos;

        MediaData output = MediaData::createVideo(std::move(compressedData), outputFormat);
        output.timestamp = input.timestamp;

        return output;
    }

    void TheoraProcessor::InitializeEncoder(const MediaData& firstFrame) {
        const VideoFormat& format = std::get<VideoFormat>(firstFrame.format);

        th_info info;
        th_info_init(&info);

        info.frame_width = format.width;
        info.frame_height = format.height;
        info.pic_width = format.width;
        info.pic_height = format.height;
        info.pic_x = 0;
        info.pic_y = 0;
        info.colorspace = TH_CS_ITU_REC_470BG;
        info.pixel_fmt = TH_PF_420;
        info.target_bitrate = 600000;
        info.quality = 48;
        info.fps_numerator = static_cast<int>(format.frameRate);
        info.fps_denominator = 1;

        enc_state = th_encode_alloc(&info);
        if (!enc_state) {
            th_info_clear(&info);
            throw std::runtime_error("Failed to initialize Theora encoder");
        }

        th_info_clear(&info);
        isInitialized = true;
    }

    void TheoraProcessor::SetupTheoraPicture(th_ycbcr_buffer& ycbcr,
        const uint8_t* yuvData,
        int width,
        int height) {
        // Y plane
        ycbcr[0].width = width;
        ycbcr[0].height = height;
        ycbcr[0].stride = width;
        ycbcr[0].data = const_cast<unsigned char*>(yuvData);

        // U plane
        ycbcr[1].width = width / 2;
        ycbcr[1].height = height / 2;
        ycbcr[1].stride = width / 2;
        ycbcr[1].data = const_cast<unsigned char*>(yuvData + (width * height));

        // V plane
        ycbcr[2].width = width / 2;
        ycbcr[2].height = height / 2;
        ycbcr[2].stride = width / 2;
        ycbcr[2].data = const_cast<unsigned char*>(yuvData + (width * height * 5 / 4));
    }
}
