#include <fstream>

#include "media_pipeline/file_formats/mp3_format.h"
#include "media_pipeline/core/interfaces/i_file_format.h"
#include "media_pipeline/core/media_data.h"

namespace media_pipeline::file_formats {
    using core::MediaData;

    void Mp3FileFormat::WriteHeader(std::ofstream& file) {
        const char id3Header[]{
            'I', 'D', '3',          // ID3 identifier
            0x03, 0x00,             // Version 2.3.0
            0x00,                   // Flags (none set)
            0x00, 0x00, 0x00, 0x00  // Size
        };

        file.write(id3Header, sizeof(id3Header));
    }

    void Mp3FileFormat::WriteData(std::ofstream& file, const MediaData& data) {
        if (!file.is_open()) throw std::runtime_error("MP3 file not open");
        file.write(reinterpret_cast<const char*>(data.data.data()),
            data.data.size());
    }

    void Mp3FileFormat::Finalize(std::ofstream& file) {
        // Not necessary for MP3 file
    }
}
