#pragma once
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include "audio_pipeline.h"


class NetworkAudioSink : public IAudioSink {
public:
	NetworkAudioSink(const std::string& address, USHORT port);
	~NetworkAudioSink();

	void Start() override;
	void Stop() override;
	void ConsumeAudioData(const AudioData& data) override;

private:
	SOCKET sock;
	USHORT serverPort;
	std::string serverAddress;
};
