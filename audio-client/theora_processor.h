#pragma once
#include "media_pipeline.h"
#include <theora/theoraenc.h>
#include <theora/theoradec.h>


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
