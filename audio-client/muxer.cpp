#include "muxer.h"


OggMuxer::OggMuxer(std::shared_ptr<MediaQueue> mediaQueue) : 
	isRunning(false),
	mediaQueue(mediaQueue) {}

void OggMuxer::MuxerLoop() {
	while (isRunning) {
		MediaData data = mediaQueue->Pop();

		if (data.isEndOfStream) {
			// need to finalize ogg stream
			break;
		}

		switch (data.type) {
			case MediaData::Type::Audio: {
				const AudioFormat& format = data.getAudioFormat();
				handleAudioData(data, format);
				break;
			}
			case MediaData::Type::Video: {
				const VideoFormat& format = data.getVideoFormat();
				handleVideoData(data, format);
				break;
			}
		}
	}
}

void OggMuxer::Start() {
	// spin up OggMuxer on new thread
}

void OggMuxer::Stop() {

}

void OggMuxer::handleAudioData(const MediaData& data, const AudioFormat& format) {

}


void OggMuxer::handleVideoData(const MediaData& data, const VideoFormat& format) {

}
