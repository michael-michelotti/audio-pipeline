#include <stdexcept>
#include <iostream>

#include <x265.h>

#include "media_pipeline/processors/video/hevc_processor.h"
#include "media_pipeline/core/interfaces/i_media_processor.h"
#include "media_pipeline/core/interfaces/i_media_component.h"
#include "media_pipeline/core/media_data.h"

namespace media_pipeline::processors::video {
    using core::MediaData;
    using core::VideoFormat;

    HevcProcessor::HevcProcessor() 
        : isInitialized(false)
        , encoder(nullptr)
        , param(nullptr) {}

    HevcProcessor::~HevcProcessor() {
        if (encoder) {
            x265_encoder_close(encoder);
        }
        if (param) {
            x265_param_free(param);
        }
    }

    void HevcProcessor::Start() {
        isInitialized = false;
    }

    void HevcProcessor::Stop() {
        if (encoder) {
            x265_encoder_close(encoder);
            encoder = nullptr;
        }
        if (param) {
            x265_param_free(param);
            param = nullptr;
        }
        isInitialized = false;
    }

    MediaData HevcProcessor::ProcessMediaData(const MediaData& input) {
        if (!isInitialized) {
            InitializeEncoder(input);

            // Get HEVC headers (VPS, SPS, PPS)
            x265_nal* nals;
            uint32_t nalCount;
            if (x265_encoder_headers(encoder, &nals, &nalCount) < 0) {
                throw std::runtime_error("Failed to get HEVC headers");
            }

            // Combine all header NALs into one buffer
            std::vector<uint8_t> headerData;
            for (uint32_t i = 0; i < nalCount; i++) {
                size_t currentSize = headerData.size();
                headerData.resize(currentSize + nals[i].sizeBytes);

                // Write NAL data
                std::memcpy(headerData.data() + currentSize,
                    nals[i].payload,
                    nals[i].sizeBytes);
            }
            VideoFormat headerFormat = std::get<VideoFormat>(input.format);
            headerFormat.format = VideoFormat::PixelFormat::HEVC_HEADERS;
            return MediaData::createVideo(std::move(headerData), headerFormat);
        }

        const VideoFormat& format = std::get<VideoFormat>(input.format);
        x265_picture* pic_in = x265_picture_alloc();
        x265_picture_init(param, pic_in);

        // Set up picture planes (assuming YUV420P input)
        uint8_t* basePtr = const_cast<uint8_t*>(input.data.data());
        pic_in->planes[0] = basePtr;                                          // Y
        pic_in->planes[1] = basePtr + (format.width * format.height);         // U
        pic_in->planes[2] = basePtr + (format.width * format.height * 5 / 4);     // V

        pic_in->stride[0] = format.width;
        pic_in->stride[1] = format.width / 2;
        pic_in->stride[2] = format.width / 2;

        // Encode frame
        x265_picture* pic_out = x265_picture_alloc();
        x265_nal* nals;
        uint32_t nalCount;
        int frameSize = x265_encoder_encode(encoder, &nals, &nalCount, pic_in, pic_out);

        if (frameSize < 0) {
            x265_picture_free(pic_in);
            x265_picture_free(pic_out);
            throw std::runtime_error("Failed to encode frame");
        }

        // Combine all NALs into one buffer
        std::vector<uint8_t> compressedData;
        for (uint32_t i = 0; i < nalCount; i++) {
            size_t currentSize = compressedData.size();
            compressedData.resize(currentSize + nals[i].sizeBytes);
            // Write NAL data
            std::memcpy(compressedData.data() + currentSize,
                nals[i].payload,
                nals[i].sizeBytes);
        }

        // Create output MediaData
        VideoFormat outputFormat;
        outputFormat.width = format.width;
        outputFormat.height = format.height;
        outputFormat.frameRate = format.frameRate;
        outputFormat.format = VideoFormat::PixelFormat::HEVC;
        outputFormat.isKeyFrame = pic_out->sliceType == X265_TYPE_IDR ||
            pic_out->sliceType == X265_TYPE_I;

        MediaData output = MediaData::createVideo(std::move(compressedData), outputFormat);
        output.timestamp = pic_out->pts * 1000;  // Convert to microseconds

        x265_picture_free(pic_in);
        x265_picture_free(pic_out);

        return output;
    }

    void HevcProcessor::InitializeEncoder(const MediaData& firstFrame) {
        const VideoFormat& format = std::get<VideoFormat>(firstFrame.format);

        param = x265_param_alloc();
        x265_param_default_preset(param, "medium", "zerolatency");

        // Configure encoding parameters
        param->sourceWidth = format.width;
        param->sourceHeight = format.height;
        param->fpsNum = static_cast<int>(format.frameRate);
        param->fpsDenom = 1;
        param->internalCsp = X265_CSP_I420;
        param->levelIdc = 0;  // Auto-detect level
        param->bRepeatHeaders = 0;  // Don't repeat headers
        param->bAnnexB = 1;  // Use Annex B format for NAL units

        // Rate control
        param->rc.rateControlMode = X265_RC_CRF;
        param->rc.rfConstant = 23;  // CRF value (lower = better quality)

        // GOP structure
        param->keyframeMax = 250;
        param->keyframeMin = 25;

        // Initialize encoder
        encoder = x265_encoder_open(param);
        if (!encoder) {
            x265_param_free(param);
            param = nullptr;
            throw std::runtime_error("Failed to initialize HEVC encoder");
        }

        isInitialized = true;
    }
}
