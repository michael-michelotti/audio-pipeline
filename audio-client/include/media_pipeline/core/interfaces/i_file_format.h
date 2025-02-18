#pragma once
#include <fstream>

#include "media_pipeline/core/media_data.h"

class IAudioFileFormat {
public:
	virtual ~IAudioFileFormat() = default;
	virtual void WriteHeader(std::ofstream& file) = 0;
	virtual void WriteData(std::ofstream& file, const MediaData& data) = 0;
	virtual void Finalize(std::ofstream& file) = 0;
};
