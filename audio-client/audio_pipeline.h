#pragma once
#include <vector>
#include <mutex>
#include <queue>


struct AudioData {
	std::vector<uint8_t> data;
	size_t frameCount;
	bool isEndOfStream = false;
};


class IAudioComponent {
public:

	virtual ~IAudioComponent() = default;
	virtual void Start() = 0;
	virtual void Stop() = 0;
};


class IAudioSource : public IAudioComponent {
public:
	virtual AudioData GetAudioData() = 0;
};


class IAudioProcessor : public IAudioComponent {
public:
	virtual AudioData ProcessAudioData(const AudioData& input) = 0;
};


class IAudioSink : public IAudioComponent {
public:
	virtual void ConsumeAudioData(const AudioData& data) = 0;
};

class AudioQueue {
public:
    void Push(AudioData data);
    AudioData Pop();

private:
    std::queue<AudioData> queue;
    std::mutex mutex;
    std::condition_variable cv;
};

class AudioPipeline {
public:
    AudioPipeline(
        std::shared_ptr<IAudioSource> source,
        std::shared_ptr<IAudioProcessor> processor,
        std::shared_ptr<IAudioSink> sink);
    void Start();
    void Stop();

private:
    void SourceThread();
    void ProcessorThread();
    void SinkThread();

    std::shared_ptr<IAudioSource> source;
    std::shared_ptr<IAudioProcessor> processor;
    std::shared_ptr<IAudioSink> sink;

    AudioQueue rawQueue;       // Queue between source and processor
    AudioQueue processedQueue; // Queue between processor and sink

    std::thread sourceThread;
    std::thread processorThread;
    std::thread sinkThread;

    std::atomic<bool> isRunning;
};
