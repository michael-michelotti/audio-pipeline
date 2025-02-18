#pragma once
#include <media_pipeline/media_pipeline.h>

using namespace media_pipeline;

class IMuxer {
public:
	virtual ~IMuxer() = default;
	virtual void Start() = 0;
	virtual void Stop() = 0;
	virtual void handleAudioData(const MediaData& data, const AudioFormat& format) = 0;
	virtual void handleVideoData(const MediaData& data, const VideoFormat& format) = 0;
	virtual void MuxerLoop() = 0;
};
