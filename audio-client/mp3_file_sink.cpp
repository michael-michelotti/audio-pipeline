#include "mp3_file_sink.h"


Mp3AudioSink::Mp3AudioSink(const std::string& filePath)
    : filePath(filePath) {
}

void Mp3AudioSink::Start() {
    mp3OutputFile.open(filePath, std::ios::binary);
    if (!mp3OutputFile) {
        throw std::runtime_error("Failed to open MP3 output file");
    }
}

void Mp3AudioSink::Stop() {
    if (mp3OutputFile.is_open()) {
        mp3OutputFile.close();
    }
}

void Mp3AudioSink::ConsumeAudioData(const AudioData& data) {
    if (!mp3OutputFile.is_open()) {
        throw std::runtime_error("MP3 file not open");
    }
    mp3OutputFile.write(reinterpret_cast<const char*>(data.data.data()),
        data.data.size());
}

Mp3AudioSink::~Mp3AudioSink() {
    Stop();
}
