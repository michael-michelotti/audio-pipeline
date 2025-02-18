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

class WebcamSource : public IMediaSource {
public:
	WebcamSource();
	~WebcamSource();
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
