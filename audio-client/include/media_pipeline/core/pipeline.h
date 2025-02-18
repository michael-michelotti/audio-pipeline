#pragma once
#include <thread>
#include <memory>
#include <atomic>

#include "media_pipeline/core/media_queue.h"
#include "media_pipeline/core/interfaces/i_media_source.h"
#include "media_pipeline/core/interfaces/i_media_processor.h"
#include "media_pipeline/core/interfaces/i_media_sink.h"

namespace media_pipeline::core {
    class MediaPipeline {
    public:
        MediaPipeline(
            std::shared_ptr<interfaces::IMediaSource> source,
            std::shared_ptr<interfaces::IMediaProcessor> processor,
            std::shared_ptr<interfaces::IMediaSink> sink);
        void Start();
        void Stop();

    private:
        void SourceThread();
        void ProcessorThread();
        void SinkThread();

        std::shared_ptr<interfaces::IMediaSource> source;
        std::shared_ptr<interfaces::IMediaProcessor> processor;
        std::shared_ptr<interfaces::IMediaSink> sink;

        MediaQueue rawQueue;
        MediaQueue processedQueue;

        std::thread sourceThread;
        std::thread processorThread;
        std::thread sinkThread;

        std::atomic<bool> isRunning;
    };
}
