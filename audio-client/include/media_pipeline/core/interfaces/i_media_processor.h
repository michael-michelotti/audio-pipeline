#pragma once
#include "media_pipeline/core/interfaces/i_media_component.h"
#include "media_pipeline/core/media_data.h"

namespace media_pipeline::core::interfaces {
	class IMediaProcessor : public IMediaComponent {
	public:
		virtual MediaData ProcessMediaData(const MediaData& input) = 0;
	};
}
