#pragma once
#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <iostream>
#include "wasapi_audio_input_source.h"
#include "media_queue.h"

#define WASAPI_LOGGING 0

WasapiAudioInputSource::WasapiAudioInputSource()
	: deviceSampleRate(0)
	, deviceChannels(0)
	, deviceBitsPerSample(0)
	, pEnumerator(nullptr)
	, pDevice(nullptr)
	, pAudioClient(nullptr)
	, pCaptureClient(nullptr)
	, bufferFrameSize(0)
{
	HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	if (FAILED(hr)) {
		throw std::runtime_error("Failed to initialize COM");
	}

	if (!Initialize()) {
		throw std::runtime_error("Failed to initialize WASAPI input source");
	}

	if (WASAPI_LOGGING) {
		std::cout << "<WASAPI Source> Successfully initialized WASAPI audio pipeline input source" << std::endl;
	}
}

WasapiAudioInputSource::~WasapiAudioInputSource() {
	CleanupCOM();
	CoUninitialize();
}

bool WasapiAudioInputSource::Initialize() {
	HRESULT hr;

	hr = CoCreateInstance(
		__uuidof(MMDeviceEnumerator),
		nullptr,
		CLSCTX_ALL,
		__uuidof(IMMDeviceEnumerator),
		(void**)&pEnumerator
	);

	hr = pEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &pDevice);

	hr = pDevice->Activate(
		__uuidof(IAudioClient),
		CLSCTX_ALL,
		nullptr,
		(void**)&pAudioClient
	);

	WAVEFORMATEX* deviceFormat = nullptr;
	hr = pAudioClient->GetMixFormat(&deviceFormat);

	deviceSampleRate = deviceFormat->nSamplesPerSec;
	deviceChannels = deviceFormat->nChannels;
	deviceBitsPerSample = deviceFormat->wBitsPerSample;

	if (WASAPI_LOGGING) {
		std::cout << "<WASAPI Source> WASAPI audio input source is being initialized with the following device parameters:" << std::endl;
		std::cout << "Input Device format:"
			<< "\n Sample rate: " << deviceSampleRate
			<< "\n Channels: " << deviceChannels
			<< "\n Bits per sample: " << deviceBitsPerSample << std::endl;
	}

	hr = pAudioClient->Initialize(
		AUDCLNT_SHAREMODE_SHARED,
		0,
		0,
		0,
		deviceFormat,
		nullptr);

	CoTaskMemFree(deviceFormat);

	hr = pAudioClient->GetBufferSize(&bufferFrameSize);
	hr = pAudioClient->GetService(
		__uuidof(IAudioCaptureClient),
		(void**)&pCaptureClient
	);

	return true;
}

void WasapiAudioInputSource::Start() {
	if (pAudioClient) HRESULT hr = pAudioClient->Start();
	if (WASAPI_LOGGING) {
		std::cout << "<WASAPI Source> Started audio client on WASAPI audio input source" << std::endl;
	}
}

void WasapiAudioInputSource::Stop() {
	if (pAudioClient) pAudioClient->Stop();
	if (WASAPI_LOGGING) {
		std::cout << "<WASAPI Source> Stopped audio client on WASAPI audio input source" << std::endl;
	}
}


MediaData WasapiAudioInputSource::GetMediaData() {
	MediaData data;
	AudioFormat format;
	format.sampleRate = deviceSampleRate;
	format.channels = deviceChannels;
	format.format = AudioFormat::SampleFormat::PCM_FLOAT;
	data.format = format;
	data.type = MediaData::Type::Audio;

	BYTE* pData;
	UINT32 numFrames = 0;
	DWORD flags;

	// Currently assumes one frame is 32 bit float, 2 channels (64 bits per frame)
	HRESULT hr = pCaptureClient->GetNextPacketSize(&numFrames);
	if (SUCCEEDED(hr) && numFrames > 0) {
		if (WASAPI_LOGGING) {
			std::cout << "<WASAPI Source> Received audio packet with " << numFrames << " frames." << std::endl;
		}
		hr = pCaptureClient->GetBuffer(&pData, &numFrames, &flags, nullptr, nullptr);
		if (SUCCEEDED(hr)) {
			size_t byteSize = numFrames * deviceChannels * sizeof(float);
			data.data.resize(byteSize);

			// Get float pointer to the raw data
			const float* inputBuffer = reinterpret_cast<float*>(pData);
			float* outputBuffer = reinterpret_cast<float*>(data.data.data());

			// Copy left channel to both channels
			for (UINT32 i = 0; i < numFrames; i++) {
				float leftSample = inputBuffer[i * 2];     // Left channel
				outputBuffer[i * 2] = leftSample;          // Left
				outputBuffer[i * 2 + 1] = leftSample;      // Right
			}

			pCaptureClient->ReleaseBuffer(numFrames);
			if (WASAPI_LOGGING) {
				std::cout << "<WASAPI Source> Returning MediaData object with " << data.data.size() << " bytes" << std::endl;
			}
			return data;
		}
	}

	if (WASAPI_LOGGING) {
		std::cout << "<WASAPI Source> Received audio packet with no frames. Sleeping for 1ms" << std::endl;
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
	return data;
}

void WasapiAudioInputSource::CleanupCOM() {
	if (pCaptureClient) pCaptureClient->Release();
	if (pAudioClient) pAudioClient->Release();
	if (pDevice) pDevice->Release();
	if (pEnumerator) pEnumerator->Release();

	pCaptureClient = nullptr;
	pAudioClient = nullptr;
	pDevice = nullptr;
	pEnumerator = nullptr;
}
