#pragma once
#include <vector>
#include <mutex>
#include <queue>

#include "audio_pipeline.h"


void AudioQueue::Push(AudioData data) {
    std::lock_guard<std::mutex> lock(mutex);
    queue.push(std::move(data));
    cv.notify_one();
}

AudioData AudioQueue::Pop() {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, [this] { return !queue.empty(); });
    AudioData data = std::move(queue.front());
    queue.pop();
    return data;
}

AudioPipeline::AudioPipeline(
    std::shared_ptr<IAudioSource> source,
    std::shared_ptr<IAudioProcessor> processor,
    std::shared_ptr<IAudioSink> sink)
    : source(source)
    , processor(processor)
    , sink(sink)
    , isRunning(false) {
}

void AudioPipeline::Start() {
    if (isRunning) return;
    isRunning = true;

    // Start all components
    source->Start();
    processor->Start();
    sink->Start();

    // Start pipeline threads
    sourceThread = std::thread(&AudioPipeline::SourceThread, this);
    processorThread = std::thread(&AudioPipeline::ProcessorThread, this);
    sinkThread = std::thread(&AudioPipeline::SinkThread, this);
}

void AudioPipeline::Stop() {
    if (!isRunning) return;
    isRunning = false;

    // Push end-of-stream markers
    AudioData eos;
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

void AudioPipeline::SourceThread() {
    while (isRunning) {
        AudioData data = source->GetAudioData();
        rawQueue.Push(std::move(data));
    }
}

void AudioPipeline::ProcessorThread() {
    while (true) {
        AudioData data = rawQueue.Pop();
        if (data.isEndOfStream) {
            processedQueue.Push(std::move(data));
            break;
        }
        AudioData processed = processor->ProcessAudioData(data);
        processedQueue.Push(std::move(processed));
    }
}

void AudioPipeline::SinkThread() {
    while (true) {
        AudioData data = processedQueue.Pop();
        if (data.isEndOfStream) break;
        sink->ConsumeAudioData(data);
    }
}
