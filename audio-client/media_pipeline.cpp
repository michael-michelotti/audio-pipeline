#pragma once
#include <vector>
#include <mutex>
#include <queue>

#include "media_pipeline.h"
#include "media_queue.h"


MediaPipeline::MediaPipeline(
    std::shared_ptr<IMediaSource> source,
    std::shared_ptr<IMediaProcessor> processor,
    std::shared_ptr<IMediaSink> sink)
    : source(source)
    , processor(processor)
    , sink(sink)
    , isRunning(false) {
}

void MediaPipeline::Start() {
    if (isRunning) return;
    isRunning = true;

    // Start all components
    source->Start();
    processor->Start();
    sink->Start();

    // Start pipeline threads
    sourceThread = std::thread(&MediaPipeline::SourceThread, this);
    processorThread = std::thread(&MediaPipeline::ProcessorThread, this);
    sinkThread = std::thread(&MediaPipeline::SinkThread, this);
}

void MediaPipeline::Stop() {
    if (!isRunning) return;
    isRunning = false;

    // Push end-of-stream markers
    MediaData eos;
    eos.isEndOfStream = true;
    rawQueue.Push(eos);
    processedQueue.Push(eos);

    // Wait for threads to finish
    if (sourceThread.joinable()) sourceThread.join();
    if (processorThread.joinable()) processorThread.join();
    if (sinkThread.joinable()) sinkThread.join();

    // Stop all components
    source->Stop();
    processor->Stop();
    sink->Stop();
}

void MediaPipeline::SourceThread() {
    while (isRunning) {
        MediaData data = source->GetMediaData();
        rawQueue.Push(std::move(data));
    }
}

void MediaPipeline::ProcessorThread() {
    while (isRunning) {
        MediaData data = rawQueue.Pop();
        if (data.isEndOfStream) {
            processedQueue.Push(std::move(data));
            break;
        }
        MediaData processed = processor->ProcessMediaData(data);
        processedQueue.Push(std::move(processed));
    }
}

void MediaPipeline::SinkThread() {
    while (isRunning) {
        MediaData data = processedQueue.Pop();
        if (data.isEndOfStream) break;
        sink->ConsumeMediaData(data);
    }
}
