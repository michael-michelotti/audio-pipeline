#pragma once
#include <lame/lame.h>
#include "media_pipeline.h"
#include "media_queue.h"

class Mp3Processor : public IMediaProcessor {
public:
	Mp3Processor(int bitrate = 320, 
		int outputSampleRate = 44100, 
		int inputSampleRate = 48000,
		int channels = 2);
	~Mp3Processor();
	void Start() override;
	void Stop() override;
	MediaData ProcessMediaData(const MediaData& input) override;

private:
	lame_global_flags* lameFlags;

	int bitrate;
	int outputSampleRate;
	int inputSampleRate;
	int channels;
};
