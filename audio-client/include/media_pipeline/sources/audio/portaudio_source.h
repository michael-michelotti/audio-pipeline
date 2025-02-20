#pragma once
#include <mutex>

#include <portaudio.h>

#include "media_pipeline/core/interfaces/i_media_source.h"
#include "media_pipeline/core/interfaces/i_media_component.h"
#include "media_pipeline/core/media_data.h"

namespace media_pipeline::sources::audio {
    using core::interfaces::IMediaSource;
    using core::MediaData;
    using core::AudioFormat;

    class PortaudioSource : public IMediaSource {
    public:
        PortaudioSource(
            double requestedSampleRate = 48000, 
            unsigned int requestedChannels = 2,
            AudioFormat::SampleFormat requestedFormat = AudioFormat::SampleFormat::PCM_S16LE
        );
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

        void DumpDeviceInfo(const PaDeviceInfo* info);
        PaSampleFormat GetPaFormat(AudioFormat::SampleFormat format);
        void HandleFloat32Input(const float* inputBuffer, unsigned long framesPerBuffer);
        void HandleInt16Input(const int16_t* inputBuffer, unsigned long framesPerBuffer);
        void HandleInt24Input(const int32_t* inputBuffer, unsigned long framesPerBuffer);
        void HandleInt32Input(const int32_t* inputBuffer, unsigned long framesPerBuffer);
        PaStream* stream;
        
        // PA Callback circular buffer
        std::vector<uint8_t> ringBuffer;
        size_t ringBufferSize;
        size_t writePos;
        size_t readPos;
        std::mutex bufferMutex;

        // Audio input device settings
        unsigned int deviceSampleRate;
        unsigned int deviceChannels;
        AudioFormat::SampleFormat deviceFormat;
    };
}
