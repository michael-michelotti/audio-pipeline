#pragma once
#include <memory>
#include <thread>
#include <fstream>
#include <atomic>

#include <media_pipeline/media_pipeline.h>
#include <ogg/ogg.h>

#include "muxing/interfaces/i_muxer.h"

class OggMuxer : public IMuxer {
public:
	OggMuxer(std::shared_ptr<MediaQueue> mediaQueue);
	~OggMuxer();
	void Start() override;
	void Stop() override;
	void MuxerLoop() override;
	void handleAudioData(const MediaData& data, const AudioFormat& format) override;
	void handleVideoData(const MediaData& data, const VideoFormat& format) override;

private:
	std::shared_ptr<MediaQueue> mediaQueue;
	std::shared_ptr<std::ostream> output;
	std::atomic<bool> isRunning{false};
	std::thread muxerThread;

	OggFileFormat oggFormat;
	std::ofstream outputFile;

	ogg_int64_t packetNo;
	ogg_int64_t granulePos;
	ogg_stream_state oggStream;
	int oggSerialNo;
};
