#include "audio_network.h"
#include <WS2tcpip.h>
#include <vector>
#include <fstream>

#pragma comment(lib, "ws2_32.lib")

AudioSender::AudioSender() : sock(INVALID_SOCKET) {
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw std::runtime_error("WSAStartup failed");
    }
}

AudioSender::~AudioSender() {
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
    }
    WSACleanup();
}

bool AudioSender::Connect(const std::string& address, uint16_t port) {
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        return false;
    }

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, address.c_str(), &serverAddr.sin_addr);

    return connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) != SOCKET_ERROR;
    }

bool AudioSender::SendData(const void* data, size_t size) {
    // First send the size of the data
    uint32_t dataSize = static_cast<uint32_t>(size);
    if (send(sock, (char*)&dataSize, sizeof(dataSize), 0) != sizeof(dataSize)) {
        return false;
    }

    // Then send the actual data
    const char* buffer = static_cast<const char*>(data);
    size_t totalSent = 0;
    while (totalSent < size) {
        int sent = send(sock, buffer + totalSent, size - totalSent, 0);
        if (sent == SOCKET_ERROR) {
            return false;
        }
        totalSent += sent;
    }
    return true;
}
