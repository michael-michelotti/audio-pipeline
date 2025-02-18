#pragma once

#include "media_pipeline/core/media_data.h"
#include "media_pipeline/core/interfaces/i_media_component.h"

class IMediaSource : public IMediaComponent {
public:
	virtual MediaData GetMediaData() = 0;
};
