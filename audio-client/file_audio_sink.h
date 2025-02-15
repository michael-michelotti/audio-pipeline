#pragma once
#include <fstream>
#include <string>
#include "media_pipeline.h"
#include "file_format.h"


class FileAudioSink : public IMediaSink {
public:
	FileAudioSink(const std::string& filePath, std::unique_ptr<IAudioFileFormat> format);
	~FileAudioSink();
	void Start() override;
	void Stop() override;
	void ConsumeMediaData(const MediaData& data) override;

private:
	std::ofstream outputFile;
	std::unique_ptr<IAudioFileFormat> format;
	std::string filePath;
};
