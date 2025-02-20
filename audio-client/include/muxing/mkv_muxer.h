#pragma once
#include <memory>
#include <fstream>
#include <atomic>
#include <thread>

#include <ebml/EbmlHead.h>
#include <ebml/EbmlStream.h>
#include <ebml/EbmlVoid.h>
#include <ebml/MemIOCallback.h>
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

using namespace media_pipeline;

class MkvMuxer : public IMuxer {
public:
	MkvMuxer(std::shared_ptr<MediaQueue> mediaQueue);
	~MkvMuxer();
	void Start() override;
	void Stop() override;
	void MuxerLoop() override;
	void handleAudioData(const MediaData& data, const AudioFormat& format);
	void handleVideoData(const MediaData& data, const VideoFormat& format);
	void handleAudioData(const MediaData& data, libmatroska::KaxCluster* cluster);
	void handleVideoData(const MediaData& data, libmatroska::KaxCluster* cluster);

private:
	void InitializeMkvStructure();
	void WriteEbmlHeader();
	void FinalizeMkvFile();

	std::shared_ptr<MediaQueue> mediaQueue;
	libebml::MemIOCallback outputFile;
	std::thread muxerThread;
	std::atomic<bool> isRunning{ false };
	std::atomic<bool> headersSet{ false };

	libmatroska::KaxSegment* segment;
	std::unique_ptr<libmatroska::KaxTrackEntry> videoTrack;
	std::unique_ptr<libmatroska::KaxTrackEntry> audioTrack;
	std::unique_ptr<libmatroska::KaxTracks> tracks;

	uint8_t videoTrackNumber;
	uint8_t audioTrackNumber;
	uint64_t timestampScale;
	uint64_t lastTimestamp;
};
