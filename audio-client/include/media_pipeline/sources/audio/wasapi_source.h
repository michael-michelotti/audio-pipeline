#pragma once
#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <Windows.h>

#include "media_pipeline/core/interfaces/i_media_component.h"
#include "media_pipeline/core/interfaces/i_media_source.h"
#include "media_pipeline/core/media_data.h"

namespace media_pipeline::sources::audio {
	using core::interfaces::IMediaSource;
	using core::MediaData;

	class WasapiSource : public IMediaSource {
	public:
		WasapiSource();
		~WasapiSource();
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
}
