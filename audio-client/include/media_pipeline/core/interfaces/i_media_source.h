#pragma once

#include "media_pipeline/core/media_data.h"
#include "media_pipeline/core/interfaces/i_media_component.h"

namespace media_pipeline::core::interfaces {
	class IMediaSource : public IMediaComponent {
	public:
		virtual MediaData GetMediaData() = 0;
	};
}
