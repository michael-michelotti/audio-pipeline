#pragma once
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "mmdeviceapi.h"
#include "Audioclient.h"
#include "media_pipeline.h"


class WasapiAudioInputSource : public IMediaSource {
public:
	WasapiAudioInputSource();
	~WasapiAudioInputSource() override;
	void Start() override;
	void Stop() override;
	MediaData GetMediaData() override;

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
