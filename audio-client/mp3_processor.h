#pragma once
#include <lame/lame.h>
#include "audio_pipeline.h"

class Mp3Processor : public IAudioProcessor {
public:
	Mp3Processor(int bitrate = 320, 
		int outputSampleRate = 44100, 
		int inputSampleRate = 48000,
		int channels = 2);
	~Mp3Processor();
	void Start() override;
	void Stop() override;
	AudioData ProcessAudioData(const AudioData& input) override;

private:
	lame_global_flags* lameFlags;

	int bitrate;
	int outputSampleRate;
	int inputSampleRate;
	int channels;
};
