#pragma once
#include <string>

#include <WinSock2.h>

#include "media_pipeline/core/interfaces/i_media_sink.h"
#include "media_pipeline/core/interfaces/i_media_component.h"
#include "media_pipeline/core/media_data.h"

namespace media_pipeline::sinks::general {
	using core::interfaces::IMediaSink;
	using core::MediaData;

	class NetworkSink : public IMediaSink {
	public:
		NetworkSink(const std::string& address, USHORT port);
		~NetworkSink();

		void Start() override;
		void Stop() override;
		void ConsumeMediaData(const MediaData& data) override;

	private:
		SOCKET sock;
		USHORT serverPort;
		std::string serverAddress;
	};
}
