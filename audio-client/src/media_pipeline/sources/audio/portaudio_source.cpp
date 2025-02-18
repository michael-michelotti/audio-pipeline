#include <mutex>
#include <iostream>

#include <portaudio.h>

#include "media_pipeline/sources/audio/portaudio_source.h"
#include "media_pipeline/core/interfaces/i_media_source.h"
#include "media_pipeline/core/interfaces/i_media_component.h"
#include "media_pipeline/core/media_data.h"

namespace media_pipeline::sources::audio {
	using core::MediaData;

	PortaudioSource::PortaudioSource()
		: stream(nullptr)
		, ringBufferSize(preferredBufferFrames * 8)
		, writePos(0)
		, readPos(0)
		, deviceSampleRate(48000)
		, deviceChannels(2)
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

		deviceSampleRate = static_cast<unsigned int>(deviceInfo->defaultSampleRate);
		std::cout << "Device sample rate: " << deviceSampleRate << std::endl;
		deviceChannels = 2;

		PaStreamParameters inputParams;
		inputParams.device = inputDevice;
		inputParams.channelCount = deviceChannels;
		inputParams.sampleFormat = paFloat32;
		inputParams.suggestedLatency = deviceInfo->defaultHighInputLatency;
		inputParams.hostApiSpecificStreamInfo = nullptr;

		err = Pa_OpenStream(
			&stream,
			&inputParams,
			nullptr,  // No output
			48000,		// hard coded 48 kHz at the moment
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

		size_t available;
		if (writePos >= readPos) {
			available = writePos - readPos;
		}
		else {
			available = ringBuffer.size() - (readPos - writePos);
		}

		size_t availableFrames = available / deviceChannels;

		static int underrunCount = 0;
		if (availableFrames < preferredBufferFrames) {
			underrunCount++;
			std::cout << "Underrun #" << underrunCount
				<< ": availableFrames=" << availableFrames
				<< ", writePos=" << writePos
				<< ", readPos=" << readPos << std::endl;

			// data.frameCount = 0;
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			return data;
		}
		else if (availableFrames >= preferredBufferFrames) {
			std::cout << "Successful read: availableFrames=" << availableFrames << std::endl;
			// data.frameCount = preferredBufferFrames;
			size_t byteSize = preferredBufferFrames * deviceChannels * sizeof(float);
			data.data.resize(byteSize);

			float* outputBuffer = reinterpret_cast<float*>(data.data.data());

			// Copy data from ring buffer
			for (size_t i = 0; i < preferredBufferFrames * deviceChannels; i++) {
				outputBuffer[i] = ringBuffer[readPos];
				readPos = (readPos + 1) % ringBuffer.size();
			}
		}

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
		const float* input = static_cast<const float*>(inputBuffer);

		if (input == nullptr) {
			return paContinue;
		}

		std::lock_guard<std::mutex> lock(self->bufferMutex);

		for (unsigned long i = 0; i < framesPerBuffer; i++) {
			float leftSample = input[i * self->deviceChannels];
			size_t bufferIndex = (self->writePos + i * 2) % self->ringBuffer.size();
			self->ringBuffer[bufferIndex] = leftSample;
			self->ringBuffer[bufferIndex + 1] = leftSample;
		}
		self->writePos = (self->writePos + framesPerBuffer * 2) % self->ringBuffer.size();

		return paContinue;
	}
}
