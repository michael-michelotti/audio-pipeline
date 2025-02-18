#define MUXER_SINK_LOGGING 0

#include <memory>
#include <atomic>
#include <iostream>

#include "media_pipeline/sinks/general/muxer_sink.h"
#include "media_pipeline/core/interfaces/i_media_sink.h"
#include "media_pipeline/core/interfaces/i_media_component.h"
#include "media_pipeline/core/media_data.h"
#include "media_pipeline/core/media_queue.h"

namespace media_pipeline::sinks::general {
	using core::MediaQueue;
	using core::MediaData;

	MuxerSink::MuxerSink(std::shared_ptr<MediaQueue> mediaQueue)
		: mediaQueue(mediaQueue) {

	}

	void MuxerSink::ConsumeMediaData(const MediaData& data) {
		if (MUXER_SINK_LOGGING) {
			std::cout << "Received audio packet with " << data.data.size() << " bytes." << std::endl;
		}
		mediaQueue->Push(data);

		if (MUXER_SINK_LOGGING) {
			std::cout << "Pushed data to muxer media queue. Current queue size: " << mediaQueue->queue.size() << std::endl;
		}
	}

	void MuxerSink::Start() {
		isRunning = true;
	}

	void MuxerSink::Stop() {
		isRunning = false;
	}
}
