#pragma once
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wmcodecdsp.h>
#pragma comment(lib, "mfplat")
#pragma comment(lib, "mfuuid")
#pragma comment(lib, "mf")
#pragma comment(lib, "mfreadwrite")

#include "media_pipeline/core/interfaces/i_media_source.h"
#include "media_pipeline/core/interfaces/i_media_component.h"
#include "media_pipeline/core/media_data.h"

namespace media_pipeline::sources::video {
	using core::interfaces::IMediaSource;
	using core::MediaData;

	class WmfSource : public IMediaSource {
	public:
		WmfSource();
		~WmfSource();
		void Start() override;
		void Stop() override;
		MediaData GetMediaData() override;

	private:
		IMFSourceReader* pReader;
		bool isInitialized;
		UINT32 width;
		UINT32 height;
		float frameRate;
		GUID subtype;
	};
}
