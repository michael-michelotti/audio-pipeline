#pragma once
#include <string>

#include <WinSock2.h>

#include "media_pipeline/core/interfaces/i_media_sink.h"
#include "media_pipeline/core/interfaces/i_media_component.h"
#include "media_pipeline/core/media_data.h"


class NetworkAudioSink : public IMediaSink {
public:
	NetworkAudioSink(const std::string& address, USHORT port);
	~NetworkAudioSink();

	void Start() override;
	void Stop() override;
	void ConsumeMediaData(const MediaData& data) override;

private:
	SOCKET sock;
	USHORT serverPort;
	std::string serverAddress;
};
