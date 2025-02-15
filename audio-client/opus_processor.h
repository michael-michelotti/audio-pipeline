#pragma once
#include "opus/opus.h"
#include "media_pipeline.h"

class OpusProcessor : public IMediaProcessor {
public:
	OpusProcessor(int bitrate = 128,
		int inputSampleRate = 48000,
		int channels = 2,
		int frameSize = 480);
	~OpusProcessor();
	void Start() override;
	void Stop() override;
	MediaData ProcessMediaData(const MediaData& input) override;

private:
	OpusEncoder* encoder;

	int bitrate;
	int inputSampleRate;
	int channels;
	int frameSize;	
};
