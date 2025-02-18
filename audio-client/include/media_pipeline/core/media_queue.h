#pragma once
#include <mutex>
#include <queue>

#include "media_pipeline/core/media_data.h"

namespace media_pipeline::core {
	class MediaQueue {
	public:
		void Push(MediaData data);
		MediaData Pop();
		std::queue<MediaData> queue;

	private:
		std::mutex mutex;
		std::condition_variable cv;
	};
}
