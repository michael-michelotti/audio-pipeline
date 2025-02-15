#pragma once
#include "media_pipeline.h"
#include "media_queue.h"


class MuxerAudioSink : public IMediaSink {
public:
	MuxerAudioSink(std::shared_ptr<MediaQueue> mediaQueue);
	void ConsumeMediaData(const MediaData& data) override;
	void Start() override;
	void Stop() override;

private:
	std::shared_ptr<MediaQueue> mediaQueue;
	std::atomic<bool> isRunning{ false };
};
