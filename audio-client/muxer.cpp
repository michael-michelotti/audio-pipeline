#include "muxer.h"
#include "file_format.h"
#include <thread>
#include <iostream>
#include <fstream>


OggMuxer::OggMuxer(std::shared_ptr<MediaQueue> mediaQueue) : 
	isRunning(false),
	mediaQueue(mediaQueue),
	packetNo(0),
	granulePos(0),
	oggSerialNo (0) {

	srand(time(NULL));
	int serialNo = rand();
	if (ogg_stream_init(&oggStream, serialNo) < 0) {
		throw std::runtime_error("Failed to initialize Ogg stream");
	}
}

OggMuxer::~OggMuxer() {
	ogg_stream_clear(&oggStream);
}

void OggMuxer::MuxerLoop() {
	while (isRunning) {
		MediaData data = mediaQueue->Pop();
		// std::cout << "Size of mux queue: " << mediaQueue->queue.size() << std::endl;;

		if (data.isEndOfStream) {
			// need to finalize ogg stream
			break;
		}

		switch (data.type) {
			case MediaData::Type::Audio: {
				//std::cout << "<OggMuxer> Media is type: audio" << std::endl;
				uint32_t frameCount;
				std::memcpy(&frameCount, data.data.data(), sizeof(frameCount));
				//std::cout << "<OggMuxer> Popped MediaData with: " << frameCount << " frames." << std::endl;
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
	oggFormat.Finalize(outputFile);
}

void OggMuxer::Start() {
	// spin up OggMuxer on new thread
	isRunning = true;
	muxerThread = std::thread(&OggMuxer::MuxerLoop, this);
	outputFile.open("test.ogg", std::ios::binary);
	if (!outputFile) {
		throw std::runtime_error("Failed to open OGG output file");
	}
	oggFormat.WriteHeader(outputFile);
}

void OggMuxer::Stop() {
	isRunning = false;
	if (muxerThread.joinable()) {
		muxerThread.join();
	}
	outputFile.close();
}

void OggMuxer::handleAudioData(const MediaData& data, const AudioFormat& format) {
	oggFormat.WriteData(outputFile, data);
}


void OggMuxer::handleVideoData(const MediaData& data, const VideoFormat& format) {
	if (format.format == VideoFormat::PixelFormat::THEORA_HEADERS) {
		oggFormat.WriteTheoraHeaders(outputFile, data);
	}
	else {
		oggFormat.WriteData(outputFile, data);
	}
}
