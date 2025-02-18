#pragma once
#include "media_pipeline/core/media_data.h"
#include "media_pipeline/core/interfaces/i_media_component.h"

class IMediaSink : public IMediaComponent {
public:
	virtual void ConsumeMediaData(const MediaData& data) = 0;
};
