#pragma once
#include <fstream>

#include "media_pipeline/core/interfaces/i_file_format.h"
#include "media_pipeline/core/media_data.h"

class Mp3FileFormat : public IAudioFileFormat {
public:
	void WriteHeader(std::ofstream& file) override;
	void WriteData(std::ofstream& file, const MediaData& data) override;
	void Finalize(std::ofstream& file) override;
};
