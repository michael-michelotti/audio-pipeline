#pragma once
#include <fstream>
#include <string>
#include "audio_pipeline.h"


class Mp3AudioSink: public IAudioSink {
public:
	Mp3AudioSink(const std::string& filePath);
	~Mp3AudioSink();
	void Start() override;
	void Stop() override;
	void ConsumeAudioData(const AudioData& data) override;

private:
	std::ofstream mp3OutputFile;
	std::string filePath;
};
