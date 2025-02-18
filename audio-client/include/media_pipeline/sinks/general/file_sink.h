#pragma once
#include <fstream>
#include <string>

#include "media_pipeline/core/interfaces/i_media_sink.h"
#include "media_pipeline/core/interfaces/i_media_component.h"
#include "media_pipeline/core/interfaces/i_file_format.h"
#include "media_pipeline/core/media_data.h"

class FileSink : public IMediaSink {
public:
	FileSink(const std::string& filePath, std::unique_ptr<IAudioFileFormat> format);
	~FileSink();
	void Start() override;
	void Stop() override;
	void ConsumeMediaData(const MediaData& data) override;

private:
	std::ofstream outputFile;
	std::unique_ptr<IAudioFileFormat> format;
	std::string filePath;
};
