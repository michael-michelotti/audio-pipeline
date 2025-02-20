#pragma once
#include <x265.h>

#include "media_pipeline/core/interfaces/i_media_processor.h"
#include "media_pipeline/core/interfaces/i_media_component.h"
#include "media_pipeline/core/media_data.h"

namespace media_pipeline::processors::video {
    using core::MediaData;
    using core::VideoFormat;
    using core::interfaces::IMediaProcessor;

    class HevcProcessor : public IMediaProcessor {
    public:
        HevcProcessor();
        ~HevcProcessor();
        void Start() override;
        void Stop() override;
        MediaData ProcessMediaData(const MediaData& input) override;

    private:
        void InitializeEncoder(const MediaData& firstFrame);

        bool isInitialized;
        x265_encoder* encoder;
        x265_param* param;
    };
}
