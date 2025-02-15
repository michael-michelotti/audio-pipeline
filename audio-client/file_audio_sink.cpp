#include "file_audio_sink.h"
#include <iostream>


FileAudioSink::FileAudioSink(const std::string& filePath, std::unique_ptr<IAudioFileFormat> format)
    : filePath(filePath), format(std::move(format)) {
}

void FileAudioSink::Start() {
    outputFile.open(filePath, std::ios::binary);
    if (!outputFile) {
        throw std::runtime_error("Failed to open MP3 output file");
    }
    format->WriteHeader(outputFile);
}

void FileAudioSink::Stop() {
    format->Finalize(outputFile);
    if (outputFile.is_open()) {
        outputFile.close();
    }
}

void FileAudioSink::ConsumeMediaData(const MediaData& data) {
    format->WriteData(outputFile, data);
}

FileAudioSink::~FileAudioSink() {
    try {
        if (outputFile.is_open()) {
            outputFile.close();
        }
    }
    catch (const std::exception& e) {
        std::cout << "Error closing audio file sink" << std::endl;
    }
}
