#include <iostream>
#include <vector>
#include <stdexcept>

#include "audio_server.h"

#pragma comment(lib, "ws2_32.lib")


AudioServer::AudioServer() : listenSock(INVALID_SOCKET), clientSock(INVALID_SOCKET) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw std::runtime_error("WSAStartup failed");
    }
}

AudioServer::~AudioServer() {
    if (clientSock != INVALID_SOCKET) closesocket(clientSock);
    if (listenSock != INVALID_SOCKET) closesocket(listenSock);
    WSACleanup();
}

bool AudioServer::Start(uint16_t port) {
    listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET) {
        return false;
    }

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(listenSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        return false;
    }

    if (listen(listenSock, 1) == SOCKET_ERROR) {
        return false;
    }

    return true;
}

void AudioServer::AcceptAndHandle() {
    std::cout << "Waiting for connection..." << std::endl;

    clientSock = accept(listenSock, nullptr, nullptr);
    if (clientSock == INVALID_SOCKET) {
        return;
    }

    std::cout << "Client connected. Receiving data..." << std::endl;

    while (true) {
        // Receive size of incoming data
        uint32_t dataSize;
        if (recv(clientSock, (char*)&dataSize, sizeof(dataSize), 0) != sizeof(dataSize)) {
            break;
        }

        // Receive the actual data
        std::vector<uint8_t> buffer(dataSize);
        size_t totalReceived = 0;
        while (totalReceived < dataSize) {
            int received = recv(clientSock, (char*)buffer.data() + totalReceived,
                dataSize - totalReceived, 0);
            if (received <= 0) {
                return;
            }
            totalReceived += received;
        }

        // Print the size of received data chunk
        std::cout << "Received chunk of size: " << dataSize << " bytes\n";
    }
}
