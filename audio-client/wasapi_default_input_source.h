#pragma once
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "audio_pipeline.h"


class WasapiDefaultInputSource : IAudioSource {
public:
	WasapiDefaultInputSource();
	~WasapiDefaultInputSource() override;
	void Start() override;
	void Stop() override;
	AudioData GetAudioData() override;

private:
	bool Initialize();
	void CleanupCOM();

	DWORD deviceSampleRate;
	WORD deviceChannels;
	WORD deviceBitsPerSample;

	IMMDeviceEnumerator* pEnumerator;
	IMMDevice* pDevice;
	IAudioClient* pAudioClient;
	IAudioCaptureClient* pCaptureClient;

	UINT32 bufferFrameSize;
};
