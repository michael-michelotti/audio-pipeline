#include "muxer_audio_sink.h"
#include <iostream>

#define MUXER_SINK_LOGGING 0

MuxerAudioSink::MuxerAudioSink(std::shared_ptr<MediaQueue> mediaQueue) 
	: mediaQueue(mediaQueue) {

}


void MuxerAudioSink::ConsumeMediaData(const MediaData& data) {
	if (MUXER_SINK_LOGGING) {
		std::cout << "Received audio packet with " << data.data.size() << " bytes." << std::endl;
	}
	mediaQueue->Push(data);

	if (MUXER_SINK_LOGGING) {
		std::cout << "Pushed data to muxer media queue. Current queue size: " << mediaQueue->queue.size() << std::endl;
	}

}


void MuxerAudioSink::Start() {
	isRunning = true;
}


void MuxerAudioSink::Stop() {
	isRunning = false;
}
