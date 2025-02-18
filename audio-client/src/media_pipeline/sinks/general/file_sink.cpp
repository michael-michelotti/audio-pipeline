#include <fstream>
#include <string>
#include <iostream>

#include "media_pipeline/sinks/general/file_sink.h"
#include "media_pipeline/core/interfaces/i_media_sink.h"
#include "media_pipeline/core/interfaces/i_media_component.h"
#include "media_pipeline/core/interfaces/i_file_format.h"
#include "media_pipeline/core/media_data.h"

namespace media_pipeline::sinks::general {
    using core::MediaData;
    using core::interfaces::IFileFormat;

    FileSink::FileSink(const std::string& filePath, std::unique_ptr<IFileFormat> format)
        : filePath(filePath), format(std::move(format)) {
    }

    void FileSink::Start() {
        outputFile.open(filePath, std::ios::binary);
        if (!outputFile) {
            throw std::runtime_error("Failed to open MP3 output file");
        }
        format->WriteHeader(outputFile);
    }

    void FileSink::Stop() {
        format->Finalize(outputFile);
        if (outputFile.is_open()) {
            outputFile.close();
        }
    }

    void FileSink::ConsumeMediaData(const MediaData& data) {
        format->WriteData(outputFile, data);
    }

    FileSink::~FileSink() {
        try {
            if (outputFile.is_open()) {
                outputFile.close();
            }
        }
        catch (const std::exception& e) {
            std::cout << "Error closing audio file sink" << std::endl;
        }
    }
}
