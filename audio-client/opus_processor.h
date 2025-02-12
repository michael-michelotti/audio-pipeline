#pragma once
#include "opus/opus.h"
#include "audio_pipeline.h"

class OpusProcessor : public IAudioProcessor {
public:
	OpusProcessor(int bitrate = 128,
		int inputSampleRate = 48000,
		int channels = 2,
		int frameSize = 960);
	~OpusProcessor();
	void Start() override;
	void Stop() override;
	AudioData ProcessAudioData(const AudioData& input) override;

private:
	OpusEncoder* encoder;

	int bitrate;
	int inputSampleRate;
	int channels;
	int frameSize;
};
