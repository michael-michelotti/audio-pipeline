#pragma once
#include <fstream>
#include <string>

#include "media_pipeline/core/interfaces/i_media_sink.h"
#include "media_pipeline/core/interfaces/i_media_component.h"
#include "media_pipeline/core/interfaces/i_file_format.h"
#include "media_pipeline/core/media_data.h"

namespace media_pipeline::sinks::general {
	using core::interfaces::IMediaSink;
	using core::interfaces::IFileFormat;
	using core::MediaData;

	class FileSink : public IMediaSink {
	public:
		FileSink(const std::string& filePath, std::unique_ptr<IFileFormat> format);
		~FileSink();
		void Start() override;
		void Stop() override;
		void ConsumeMediaData(const MediaData& data) override;

	private:
		std::ofstream outputFile;
		std::unique_ptr<IFileFormat> format;
		std::string filePath;
	};
}
