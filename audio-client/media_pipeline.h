#pragma once
#include <vector>
#include <mutex>
#include <queue>
#include "media_queue.h"


class IMediaComponent {
public:

	virtual ~IMediaComponent() = default;
	virtual void Start() = 0;
	virtual void Stop() = 0;
};


class IMediaSource : public IMediaComponent {
public:
	virtual MediaData GetMediaData() = 0;
};


class IMediaProcessor : public IMediaComponent {
public:
	virtual MediaData ProcessMediaData(const MediaData& input) = 0;
};


class IMediaSink : public IMediaComponent {
public:
	virtual void ConsumeMediaData(const MediaData& data) = 0;
};


class MediaPipeline {
public:
    MediaPipeline(
        std::shared_ptr<IMediaSource> source,
        std::shared_ptr<IMediaProcessor> processor,
        std::shared_ptr<IMediaSink> sink);
    void Start();
    void Stop();

private:
    void SourceThread();
    void ProcessorThread();
    void SinkThread();

    std::shared_ptr<IMediaSource> source;
    std::shared_ptr<IMediaProcessor> processor;
    std::shared_ptr<IMediaSink> sink;

    MediaQueue rawQueue;
    MediaQueue processedQueue;

    std::thread sourceThread;
    std::thread processorThread;
    std::thread sinkThread;

    std::atomic<bool> isRunning;
};
