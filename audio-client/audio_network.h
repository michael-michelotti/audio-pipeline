#pragma once
#include <WinSock2.h>
#include <string>


class AudioSender {
public:
    AudioSender();
    ~AudioSender();

    bool Connect(const std::string& address, uint16_t port);
    bool SendData(const void* data, size_t size);

private:
    SOCKET sock;
};
