#pragma once
#include <memory>
#include <atomic>

#include "media_pipeline/core/interfaces/i_media_sink.h"
#include "media_pipeline/core/interfaces/i_media_component.h"
#include "media_pipeline/core/media_data.h"
#include "media_pipeline/core/media_queue.h"

namespace media_pipeline::sinks::general {
	using core::MediaQueue;
	using core::MediaData;
	using core::interfaces::IMediaSink;

	class MuxerSink : public IMediaSink {
	public:
		MuxerSink(std::shared_ptr<MediaQueue> mediaQueue);
		void ConsumeMediaData(const MediaData& data) override;
		void Start() override;
		void Stop() override;

	private:
		std::shared_ptr<MediaQueue> mediaQueue;
		std::atomic<bool> isRunning{ false };
	};
}
