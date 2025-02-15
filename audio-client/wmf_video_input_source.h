#pragma once
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wmcodecdsp.h>
#pragma comment(lib, "mfplat")
#pragma comment(lib, "mfuuid")
#pragma comment(lib, "mf")
#pragma comment(lib, "mfreadwrite")

#include "media_pipeline.h"
#include "media_queue.h"


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
