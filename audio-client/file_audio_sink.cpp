#include "file_audio_sink.h"


void Mp3FileFormat::WriteHeader(std::ofstream& file) {
    const char id3Header[]{
        'I', 'D', '3',          // ID3 identifier
        0x03, 0x00,             // Version 2.3.0
        0x00,                   // Flags (none set)
        0x00, 0x00, 0x00, 0x00  // Size
    };

    file.write(id3Header, sizeof(id3Header));
}

void Mp3FileFormat::WriteData(std::ofstream& file, const AudioData& data) {
    if (!file.is_open()) throw std::runtime_error("MP3 file not open");
    file.write(reinterpret_cast<const char*>(data.data.data()),
        data.data.size());
}

void Mp3FileFormat::Finalize(std::ofstream& file) {
    // Not necessary for MP3 file
}


FileAudioSink::FileAudioSink(const std::string& filePath, std::unique_ptr<IAudioFileFormat> format)
    : filePath(filePath), format(std::move(format)) {
}

void FileAudioSink::Start() {
    outputFile.open(filePath, std::ios::binary);
    if (!outputFile) {
        throw std::runtime_error("Failed to open MP3 output file");
    }
}

void FileAudioSink::Stop() {
    if (outputFile.is_open()) {
        outputFile.close();
    }
}

void FileAudioSink::ConsumeAudioData(const AudioData& data) {
    format->WriteData(outputFile, data);
}

FileAudioSink::~FileAudioSink() {
    Stop();
}
