#pragma once
#include <ogg/ogg.h>

#include "media_pipeline/core/interfaces/i_file_format.h"
#include "media_pipeline/core/media_data.h"

namespace media_pipeline::file_formats {
	using media_pipeline::core::interfaces::IFileFormat;
	using media_pipeline::core::MediaData;

	class OggFileFormat : public IFileFormat {
		static constexpr int OPUS_PRESKIP = 312;

	public:
		OggFileFormat();
		~OggFileFormat();
		void WriteHeader(std::ofstream& file) override;
		void WriteData(std::ofstream& file, const MediaData& data) override;
		void WriteAudioData(std::ofstream& file, const MediaData& data);
		void WriteVideoData(std::ofstream& file, const MediaData& data);
		void Finalize(std::ofstream& file) override;
		void WriteTheoraHeaders(std::ofstream& file, const MediaData& data);

	private:
		void WriteOggPages(std::ofstream& file, ogg_stream_state& stream);
		void FlushOggPages(std::ofstream& file, ogg_stream_state& stream);
		void WriteOpusHead(std::ofstream& file);
		void WriteOpusTags(std::ofstream& file);
		ogg_packet GenerateEmptyPacket();

		bool headersSet = false;
		ogg_int64_t audioPacketNo;
		ogg_int64_t audioGranulePos;
		ogg_stream_state audioStream;
		ogg_int64_t videoPacketNo;
		ogg_int64_t videoGranulePos;
		ogg_int64_t relativePackets;
		ogg_int64_t totalPackets;
		ogg_stream_state videoStream;
		int audioSerialNo;
		int videoSerialNo;
	};
}
