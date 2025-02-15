#pragma once
#include <fstream>
#include <ogg/ogg.h>
#include <theora/theora.h>
#include "media_queue.h"


class IAudioFileFormat {
public:
	virtual ~IAudioFileFormat() = default;
	virtual void WriteHeader(std::ofstream& file) = 0;
	virtual void WriteData(std::ofstream& file, const MediaData& data) = 0;
	virtual void Finalize(std::ofstream& file) = 0;
};

class Mp3FileFormat : public IAudioFileFormat {
public:
	void WriteHeader(std::ofstream& file) override;
	void WriteData(std::ofstream& file, const MediaData& data) override;
	void Finalize(std::ofstream& file) override;
};

class OggFileFormat : public IAudioFileFormat {
	static constexpr int OPUS_PRESKIP = 312;
public:
	OggFileFormat();
	~OggFileFormat();
	void WriteHeader(std::ofstream& file) override;
	void WriteData(std::ofstream& file, const MediaData& data) override;
	void WriteAudioData(std::ofstream& file, const MediaData& data);
	void WriteVideoData(std::ofstream& file, const MediaData& data);
	void Finalize(std::ofstream& file) override;
	void WriteTheoraHeaders(std::ofstream& file, const MediaData& data);


private:
	void WriteOggPages(std::ofstream& file, ogg_stream_state& stream);
	void FlushOggPages(std::ofstream& file, ogg_stream_state& stream);
	void WriteOpusHead(std::ofstream& file);
	void WriteOpusTags(std::ofstream& file);

	bool headersSet = false;
	ogg_int64_t audioPacketNo;
	ogg_int64_t audioGranulePos;
	ogg_stream_state audioStream;
	ogg_int64_t videoPacketNo;
	ogg_int64_t videoGranulePos;
	ogg_int64_t lastKeyFrame = 0;
	ogg_stream_state videoStream;
	int audioSerialNo;
	int videoSerialNo;
};
