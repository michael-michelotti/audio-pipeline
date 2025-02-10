#pragma once
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>


class AudioServer {
public:
    AudioServer();
    ~AudioServer();

    bool Start(uint16_t port);

    void AcceptAndHandle();

private:
    SOCKET listenSock;
    SOCKET clientSock;
};
