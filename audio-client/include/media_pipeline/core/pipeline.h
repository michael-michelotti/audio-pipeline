#pragma once
#include <thread>
#include <memory>
#include <atomic>

#include "media_pipeline/core/media_queue.h"
#include "media_pipeline/core/interfaces/i_media_source.h"
#include "media_pipeline/core/interfaces/i_media_processor.h"
#include "media_pipeline/core/interfaces/i_media_sink.h"

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
