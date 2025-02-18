#pragma once
#include <theora/theoraenc.h>

#include "media_pipeline/core/interfaces/i_media_processor.h"
#include "media_pipeline/core/interfaces/i_media_component.h"
#include "media_pipeline/core/media_data.h"

namespace media_pipeline::processors::video {
    using core::interfaces::IMediaProcessor;
    using core::MediaData;

    class TheoraProcessor : public IMediaProcessor {
    public:
        TheoraProcessor();
        ~TheoraProcessor();
        void Start() override;
        void Stop() override;
        MediaData ProcessMediaData(const MediaData& input) override;

    private:
        void InitializeEncoder(const MediaData& firstFrame);

        void SetupTheoraPicture(th_ycbcr_buffer& ycbcr,
            const uint8_t* yuvData,
            int width,
            int height);

        th_enc_ctx* enc_state;
        bool isInitialized;
    };
}
