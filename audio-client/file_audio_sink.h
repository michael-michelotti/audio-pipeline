#pragma once
#include <fstream>
#include <string>
#include <ogg/ogg.h>
#include "audio_pipeline.h"


class IAudioFileFormat {
public:
	virtual ~IAudioFileFormat() = default;
	virtual void WriteHeader(std::ofstream& file) = 0;
	virtual void WriteData(std::ofstream& file, const AudioData& data) = 0;
	virtual void Finalize(std::ofstream& file) = 0;
};

class Mp3FileFormat : public IAudioFileFormat {
public:
	void WriteHeader(std::ofstream& file) override;
	void WriteData(std::ofstream& file, const AudioData& data) override;
	void Finalize(std::ofstream& file) override;
};

class OggFileFormat : public IAudioFileFormat {
	static constexpr int OPUS_PRESKIP = 312;
public:
	OggFileFormat();
	~OggFileFormat();
	void WriteHeader(std::ofstream& file) override;
	void WriteData(std::ofstream& file, const AudioData& data) override;
	void Finalize(std::ofstream& file) override;

private:
	ogg_int64_t packetNo;
	ogg_int64_t granulePos;
	ogg_stream_state oggStream;
	int oggSerialNo;
};

class FileAudioSink : public IAudioSink {
public:
	FileAudioSink(const std::string& filePath, std::unique_ptr<IAudioFileFormat> format);
	~FileAudioSink();
	void Start() override;
	void Stop() override;
	void ConsumeAudioData(const AudioData& data) override;

private:
	std::ofstream outputFile;
	std::unique_ptr<IAudioFileFormat> format;
	std::string filePath;
};
