#pragma once
#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <iostream>
#include "wasapi_default_input_source.h"


WasapiDefaultInputSource::WasapiDefaultInputSource()
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
}

WasapiDefaultInputSource::~WasapiDefaultInputSource() {
	CleanupCOM();
	CoUninitialize();
}

bool WasapiDefaultInputSource::Initialize() {
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

	std::cout << "Input Device format:"
		<< "\n Sample rate: " << deviceSampleRate
		<< "\n Channels: " << deviceChannels
		<< "\n Bits per sample: " << deviceBitsPerSample << std::endl;

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

void WasapiDefaultInputSource::Start() {
	if (pAudioClient) HRESULT hr = pAudioClient->Start();
}

void WasapiDefaultInputSource::Stop() {
	if (pAudioClient) pAudioClient->Stop();
}


AudioData WasapiDefaultInputSource::GetAudioData() {
	AudioData data;
	BYTE* pData;
	UINT32 numFrames = 0;
	DWORD flags;

	// Currently assumes one frame is 32 bit float, 2 channels (64 bits per frame)
	HRESULT hr = pCaptureClient->GetNextPacketSize(&numFrames);
	if (SUCCEEDED(hr) && numFrames > 0) {
		hr = pCaptureClient->GetBuffer(&pData, &numFrames, &flags, nullptr, nullptr);
		if (SUCCEEDED(hr)) {
			data.sampleCount = numFrames;
			size_t byteSize = numFrames * deviceChannels * sizeof(float);
			data.data.resize(byteSize);
			memcpy(data.data.data(), pData, byteSize);

			pCaptureClient->ReleaseBuffer(numFrames);
		}
	}

	return data;
}

void WasapiDefaultInputSource::CleanupCOM() {
	if (pCaptureClient) pCaptureClient->Release();
	if (pAudioClient) pAudioClient->Release();
	if (pDevice) pDevice->Release();
	if (pEnumerator) pEnumerator->Release();

	pCaptureClient = nullptr;
	pAudioClient = nullptr;
	pDevice = nullptr;
	pEnumerator = nullptr;
}
