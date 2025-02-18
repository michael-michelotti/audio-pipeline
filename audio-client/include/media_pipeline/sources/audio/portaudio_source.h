#pragma once
#include <mutex>

#include <portaudio.h>

#include "media_pipeline/core/interfaces/i_media_source.h"
#include "media_pipeline/core/interfaces/i_media_component.h"
#include "media_pipeline/core/media_data.h"

namespace media_pipeline::sources::audio {
    using core::interfaces::IMediaSource;
    using core::MediaData;

    class PortaudioSource : public IMediaSource {
    public:
        PortaudioSource();
        ~PortaudioSource();
        void Start() override;
        void Stop() override;
        MediaData GetMediaData() override;
    private:
        const size_t preferredBufferFrames = 480;

        static int RecordCallback(
            const void* inputBuffer,
            void* outputBuffer,
            unsigned long framesPerBuffer,
            const PaStreamCallbackTimeInfo* timeInfo,
            PaStreamCallbackFlags statusFlags,
            void* userData);

        PaStream* stream;
        std::vector<float> ringBuffer;
        size_t ringBufferSize;
        size_t writePos;
        size_t readPos;
        std::mutex bufferMutex;
        unsigned int deviceSampleRate;
        unsigned int deviceChannels;
    };
}
