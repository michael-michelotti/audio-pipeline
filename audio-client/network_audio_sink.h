#pragma once
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include "media_pipeline.h"


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
