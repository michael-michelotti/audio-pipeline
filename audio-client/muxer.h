#pragma once
#include <atomic>
#include "media_queue.h"
#include "media_pipeline.h"


class IMuxer {
public:
	virtual ~IMuxer() = default;
	virtual void Start() = 0;
	virtual void Stop() = 0;
	virtual void handleAudioData(const MediaData& data, const AudioFormat& format) = 0;
	virtual void handleVideoData(const MediaData& data, const VideoFormat& format) = 0;
	virtual void MuxerLoop() = 0;
};


class OggMuxer : public IMuxer {
public:
	OggMuxer(std::shared_ptr<MediaQueue> mediaQueue);
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
};
