#include <memory>
#include <fstream>
#include <atomic>
#include <thread>
#include <iostream>

#include <ebml/EbmlHead.h>
#include <ebml/EbmlStream.h>
#include <ebml/EbmlVoid.h>
#include <matroska/KaxSegment.h>
#include <matroska/KaxTracks.h>
#include <matroska/KaxCluster.h>
#include <matroska/KaxBlock.h>
#include <matroska/KaxBlockData.h>
#include <matroska/KaxSeekHead.h>
#include <matroska/KaxCues.h>
#include <matroska/KaxInfo.h>

#include <media_pipeline/media_pipeline.h>

#include "muxing/interfaces/i_muxer.h"
#include "muxing/mkv_muxer.h"

using namespace media_pipeline;
using namespace libmatroska;
using namespace libebml;

MkvMuxer::MkvMuxer(std::shared_ptr<MediaQueue> mediaQueue)
	: isRunning(false)
	, headersSet(false)
	, mediaQueue(mediaQueue)
	, segment(nullptr)
	, videoTrackNumber(0)
	, audioTrackNumber(0)
	, timestampScale(1000000)
	, lastTimestamp(0) 
{
	tracks = std::make_unique<KaxTracks>();
	videoTrack = std::make_unique<KaxTrackEntry>();
	audioTrack = std::make_unique<KaxTrackEntry>();
	InitializeMkvStructure();
}

MkvMuxer::~MkvMuxer() { }

void MkvMuxer::InitializeMkvStructure() {
	EbmlHead fileHeader;

	segment = new KaxSegment();
	tracks.reset(&GetChild<KaxTracks>(*segment));

	// Set up HEVC video track
	videoTrack.reset(&GetChild<KaxTrackEntry>(*tracks));
	videoTrack->SetGlobalTimecodeScale(timestampScale);
	videoTrackNumber = 1;
	GetChild<KaxTrackNumber>(*videoTrack).SetValue(videoTrackNumber);
	GetChild<KaxTrackType>(*videoTrack).SetValue(0x01);
	GetChild<KaxCodecID>(*videoTrack).SetValue("V_MPEGH/ISO/HEVC");

	// Set up MP3 audio track
	audioTrack.reset(&GetChild<KaxTrackEntry>(*tracks));
	audioTrack->SetGlobalTimecodeScale(timestampScale);
	audioTrackNumber = 2;
	GetChild<KaxTrackNumber>(*audioTrack).SetValue(audioTrackNumber);
	GetChild<KaxTrackType>(*audioTrack).SetValue(0x02);
	GetChild<KaxCodecID>(*audioTrack).SetValue("A_MPEG/L3");
}

void MkvMuxer::WriteEbmlHeader() {
	EbmlHead fileHeader;
	fileHeader.Render(outputFile);

	EbmlVoid spacer;
	spacer.SetSize(1024);
	spacer.Render(outputFile);
}

void MkvMuxer::FinalizeMkvFile() {
	KaxCues& cues = GetChild<KaxCues>(*segment);
	cues.SetGlobalTimecodeScale(timestampScale);
	cues.Render(outputFile);

	KaxInfo& info = GetChild<KaxInfo>(*segment);
	GetChild<KaxDuration>(info).SetValue(float(lastTimestamp));
	info.Render(outputFile);
}

void MkvMuxer::Start() {
	isRunning = true;
	//if (!outputFile) {
	//	throw std::runtime_error("Failed to open MKV output file");
	//}

	WriteEbmlHeader();
	muxerThread = std::thread(&MkvMuxer::MuxerLoop, this);
}

void MkvMuxer::Stop() {
	isRunning = false;
	if (muxerThread.joinable()) {
		muxerThread.join();
	}
	FinalizeMkvFile();
}

void MkvMuxer::MuxerLoop() {
	KaxCluster* cluster = nullptr;
	uint64_t clusterTimecode = 0;

	while (isRunning) {
		MediaData data = mediaQueue->Pop();

		if (data.isEndOfStream) {
			break;
		}

		KaxCues& dummyCues = GetChild<KaxCues>(*segment);

		// Create new cluster every 1 second (adjustable with timestampScale)
		if (!cluster || (data.timestamp - clusterTimecode) >= timestampScale) {
			if (cluster) {
				cluster->Render(outputFile, dummyCues);
				delete cluster;
			}
			cluster = new KaxCluster();
			cluster->SetParent(*segment);
			cluster->InitTimecode(data.timestamp, timestampScale);
			clusterTimecode = data.timestamp;
		}

		switch (data.type) {
		case MediaData::Type::Audio: {
			handleAudioData(data, cluster);
			break;
		}
		case MediaData::Type::Video: {
			handleVideoData(data, cluster);
			break;
		}
		}
	}
}

// Unused and overloaded the original handler functions
void MkvMuxer::handleAudioData(const MediaData& data, const AudioFormat& format) {}
void MkvMuxer::handleVideoData(const MediaData& data, const VideoFormat& format) {}

void MkvMuxer::handleAudioData(const MediaData& data, KaxCluster* cluster) {
	KaxBlockGroup& blockGroup = GetChild<KaxBlockGroup>(*cluster);
	KaxBlock& block = GetChild<KaxBlock>(blockGroup);

	block.SetParent(*cluster);
	DataBuffer frameBuffer(
		(binary*)data.data.data(),
		data.data.size()
	);
	block.AddFrame(*audioTrack, data.timestamp, frameBuffer);
}

void MkvMuxer::handleVideoData(const MediaData& data, KaxCluster* cluster) {
	const VideoFormat& format = data.getVideoFormat();

	if (format.format == VideoFormat::PixelFormat::HEVC_HEADERS) {
		KaxCodecPrivate& codecPrivate = GetChild<KaxCodecPrivate>(*videoTrack);
		codecPrivate.CopyBuffer(data.data.data(), data.data.size());
		tracks->Render(outputFile);
		return;
	}

	KaxBlockGroup& blockGroup = GetChild<KaxBlockGroup>(*cluster);
	KaxBlock& block = GetChild<KaxBlock>(blockGroup);

	block.SetParent(*cluster);

	DataBuffer frameBuffer(
		(binary*)data.data.data(),
		data.data.size()
	);
	block.AddFrame(*videoTrack, data.timestamp, frameBuffer);

	if (format.isKeyFrame) {
		GetChild<KaxBlockDuration>(blockGroup).SetValue(0);
		GetChild<KaxReferenceBlock>(blockGroup).SetValue(0);
	}
}
