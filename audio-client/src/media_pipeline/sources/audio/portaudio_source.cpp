#include <mutex>
#include <iostream>

#include <portaudio.h>

#include "media_pipeline/sources/audio/portaudio_source.h"
#include "media_pipeline/core/interfaces/i_media_source.h"
#include "media_pipeline/core/interfaces/i_media_component.h"
#include "media_pipeline/core/media_data.h"

static int packetCount = 0;

namespace media_pipeline::sources::audio {
	using core::MediaData;
	using core::AudioFormat;

	PortaudioSource::PortaudioSource(
		double requestedSampleRate, 
		unsigned int requestedChannels,
		AudioFormat::SampleFormat requestedFormat
	)
		: stream(nullptr)
		, ringBufferSize(preferredBufferFrames * 8)
		, writePos(0)
		, readPos(0)
		, deviceSampleRate(requestedSampleRate)
		, deviceChannels(requestedChannels)
		, deviceFormat(requestedFormat)
	{
		ringBuffer.resize(ringBufferSize * deviceChannels);
		PaError err = Pa_Initialize();
		if (err != paNoError) {
			throw std::runtime_error("Failed to initialize PortAudio input source");
		}

		PaDeviceIndex inputDevice = Pa_GetDefaultInputDevice();
		if (inputDevice == paNoDevice) {
			Pa_Terminate();
			throw std::runtime_error("Failed to initialize PortAudio input source");
		}

		const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(inputDevice);
		DumpDeviceInfo(deviceInfo);

		PaStreamParameters inputParams;
		inputParams.device = inputDevice;

		if (deviceInfo->maxInputChannels < deviceChannels){
			std::cout << deviceChannels << " channels cannot be supported by " << deviceInfo->name
				<< "\nSetting to max supported channels of " << deviceInfo->maxInputChannels << std::endl;
			deviceChannels = deviceInfo->maxInputChannels;
		}
		inputParams.channelCount = deviceChannels;
		inputParams.sampleFormat = GetPaFormat(deviceFormat);
		inputParams.suggestedLatency = deviceInfo->defaultHighInputLatency;
		inputParams.hostApiSpecificStreamInfo = nullptr;

		err = Pa_OpenStream(
			&stream,
			&inputParams,
			nullptr,	// No output parameters required
			deviceSampleRate,
			preferredBufferFrames,
			paClipOff,
			RecordCallback,
			this);

		if (err != paNoError) {
			Pa_Terminate();
			throw std::runtime_error("Failed to initialize PortAudio input source");
		}
	}

	PortaudioSource::~PortaudioSource() {
		if (stream) {
			Pa_CloseStream(stream);
		}
		Pa_Terminate();
	}

	void PortaudioSource::DumpDeviceInfo(const PaDeviceInfo* info) {
		std::cout << "PortAudio device default parameters:"
			<< "\n Name: " << info->name
			<< "\n Sample Rate: " << info->defaultSampleRate
			<< "\n Max Input Channels: " << info->maxInputChannels << std::endl;
	}

	PaSampleFormat PortaudioSource::GetPaFormat(AudioFormat::SampleFormat format) {
		switch (format) {
		case AudioFormat::SampleFormat::PCM_S16LE:
			return paInt16;
		case AudioFormat::SampleFormat::PCM_S24LE:
			return paInt24;
		case AudioFormat::SampleFormat::PCM_S32LE:
			return paInt32;
		case AudioFormat::SampleFormat::PCM_FLOAT:
			return paFloat32;
		default:
			return paInt16;  // Default to 16-bit PCM
		}
	}

	void PortaudioSource::Start() {
		if (stream) {
			PaError err = Pa_StartStream(stream);
			if (err != paNoError) {
				throw std::runtime_error("Failed to start PortAudio stream");
			}
		}
	}

	void PortaudioSource::Stop() {
		if (stream) {
			Pa_StopStream(stream);
		}
	}

	MediaData PortaudioSource::GetMediaData() {
		MediaData data;
		std::lock_guard<std::mutex> lock(bufferMutex);

		size_t bytesPerSample;
		switch (deviceFormat) {
		case AudioFormat::SampleFormat::PCM_S16LE:
			bytesPerSample = 2;
			break;
		case AudioFormat::SampleFormat::PCM_FLOAT:
		case AudioFormat::SampleFormat::PCM_S32LE:
		case AudioFormat::SampleFormat::PCM_S24LE:
			bytesPerSample = 4;
			break;
		default:
			throw std::runtime_error("Unsupported format on audio input device");
		}

		size_t available;
		if (writePos >= readPos) {
			available = writePos - readPos;
		}
		else {
			available = ringBuffer.size() - (readPos - writePos);
		}

		size_t availableFrames = available / (bytesPerSample * deviceChannels);

		if (availableFrames < preferredBufferFrames) {
			return data;
		}
		else if (availableFrames >= preferredBufferFrames) {
			size_t byteSize = preferredBufferFrames * deviceChannels * bytesPerSample;
			data.data.resize(byteSize);

			switch (deviceFormat) {
			case AudioFormat::SampleFormat::PCM_S16LE: {
				auto* output = reinterpret_cast<int16_t*>(data.data.data());
				auto* input = reinterpret_cast<int16_t*>(ringBuffer.data() + readPos);
				size_t copySize = preferredBufferFrames * deviceChannels * sizeof(int16_t);
				std::memcpy(output, input, copySize);
				break;
			}
			case AudioFormat::SampleFormat::PCM_FLOAT: {
				auto* output = reinterpret_cast<float*>(data.data.data());
				auto* input = reinterpret_cast<float*>(ringBuffer.data() + readPos);
				size_t copySize = preferredBufferFrames * deviceChannels * sizeof(float);
				std::memcpy(output, input, copySize);
				break;
			}
			default:
				throw std::runtime_error("Unsuported format on audio input device");
			}

			readPos = (readPos + byteSize) % ringBuffer.size();
		}

		AudioFormat format;
		format.sampleRate = deviceSampleRate;
		format.channels = deviceChannels;
		format.format = deviceFormat;
		data.format = format;
		return data;
	}

	int PortaudioSource::RecordCallback(
		const void* inputBuffer,
		void* outputBuffer,
		unsigned long framesPerBuffer,
		const PaStreamCallbackTimeInfo* timeInfo,
		PaStreamCallbackFlags statusFlags,
		void* userData)
	{
		PortaudioSource* self = static_cast<PortaudioSource*>(userData);
		if (inputBuffer == nullptr) {
			return paContinue;
		}

		std::lock_guard<std::mutex> lock(self->bufferMutex);

		switch (self->deviceFormat) {
		case AudioFormat::SampleFormat::PCM_FLOAT:
			self->HandleFloat32Input(static_cast<const float*>(inputBuffer), framesPerBuffer);
			break;
		case AudioFormat::SampleFormat::PCM_S16LE:
			self->HandleInt16Input(static_cast<const int16_t*>(inputBuffer), framesPerBuffer);
			break;
		case AudioFormat::SampleFormat::PCM_S24LE:
			self->HandleInt24Input(static_cast<const int32_t*>(inputBuffer), framesPerBuffer);
			break;
		case AudioFormat::SampleFormat::PCM_S32LE:
			self->HandleInt32Input(static_cast<const int32_t*>(inputBuffer), framesPerBuffer);
			break;
		default:
			return paAbort;
		}

		return paContinue;
	}

	void PortaudioSource::HandleFloat32Input(const float* inputBuffer, unsigned long framesPerBuffer) {
		auto* ringBufferFloat = reinterpret_cast<float*>(ringBuffer.data());

		for (unsigned long i = 0; i < framesPerBuffer; i++) {
			size_t bufferIndex = (writePos / sizeof(float) + i * deviceChannels) % (ringBuffer.size() / sizeof(float));
			for (unsigned int ch = 0; ch < deviceChannels; ch++) {
				ringBufferFloat[bufferIndex + ch] = inputBuffer[i * deviceChannels + ch];
			}
		}
		writePos = (writePos + framesPerBuffer * deviceChannels * sizeof(float)) % ringBuffer.size();
	}

	void PortaudioSource::HandleInt16Input(const int16_t* inputBuffer, unsigned long framesPerBuffer) {
		auto* ringBuffer16 = reinterpret_cast<int16_t*>(ringBuffer.data());

		for (unsigned long i = 0; i < framesPerBuffer; i++) {
			size_t bufferIndex = (writePos / sizeof(int16_t) + i * deviceChannels) % (ringBuffer.size() / sizeof(int16_t));
			for (unsigned int ch = 0; ch < deviceChannels; ch++) {
				ringBuffer16[bufferIndex + ch] = inputBuffer[i * deviceChannels + ch];
			}
		}
		writePos = (writePos + framesPerBuffer * deviceChannels * sizeof(int16_t)) % ringBuffer.size();
	}

	void PortaudioSource::HandleInt24Input(const int32_t* inputBuffer, unsigned long framesPerBuffer) {
		// To be implemented
	}

	void PortaudioSource::HandleInt32Input(const int32_t* inputBuffer, unsigned long framesPerBuffer) {
		// To be implemented
	}
}
