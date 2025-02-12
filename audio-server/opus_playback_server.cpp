#include "opus_playback_server.h"

#include <stdexcept>
#include <iostream>
#include <avrt.h>


// Create a static member function instead:
static DWORD WINAPI CaptureThreadProc(LPVOID param) {
	auto capture = static_cast<OpusPlaybackServer*>(param);
	capture->ConvertAndWriteAudio();
	return 0;
}


OpusPlaybackServer::OpusPlaybackServer() : 
	listenSock(INVALID_SOCKET),
	clientSock(INVALID_SOCKET),
	pEnumerator(nullptr),
	pDevice(nullptr),
	pAudioClient(nullptr),
	pRenderClient(nullptr),
	hCaptureThread(nullptr) {

	int err;
	decoder = opus_decoder_create(48000, 2, &err);

	pcmInterleaved.resize(480 * 2);

	HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	if (FAILED(hr)) {
		throw std::runtime_error("Failed to initialize COM");
	}

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		CoUninitialize();
		throw std::runtime_error("WSAStartup failed");
	}

	hr = CoCreateInstance(
		__uuidof(MMDeviceEnumerator),
		nullptr,
		CLSCTX_ALL,
		__uuidof(IMMDeviceEnumerator),
		(void**)&pEnumerator
	);
	if (FAILED(hr)) {
		throw std::runtime_error("Failed to create device enumerator");
	}

	hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
	if (FAILED(hr)) {
		throw std::runtime_error("Failed to get default audio endpoint");
	}

	hr = pDevice->Activate(
		__uuidof(IAudioClient),
		CLSCTX_ALL,
		nullptr,
		(void**)&pAudioClient
	);
	if (FAILED(hr)) {
		throw std::runtime_error("Failed to activate audio client");
	}

	WAVEFORMATEX* pwfx;
	hr = pAudioClient->GetMixFormat(&pwfx);
	if (FAILED(hr)) {
		throw std::runtime_error("Failed to get mix format");
	}

	std::cout << "Audio Format:" << std::endl
		<< "Sample Rate: " << pwfx->nSamplesPerSec << std::endl
		<< "Channels: " << pwfx->nChannels << std::endl
		<< "Bits Per Sample: " << pwfx->wBitsPerSample << std::endl
		<< "Format Tag: " << pwfx->wFormatTag << std::endl;

	hr = pAudioClient->Initialize(
		AUDCLNT_SHAREMODE_SHARED,
		AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
		2000000, // 1-second buffer
		0,
		pwfx,
		nullptr
	);
	CoTaskMemFree(pwfx);

	hr = pAudioClient->GetService(
		__uuidof(IAudioRenderClient),
		(void**)&pRenderClient
	);
	if (FAILED(hr)) {
		throw std::runtime_error("Failed to get render client");
	}
	
	hStopEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	if (hStopEvent == nullptr) {
		throw std::runtime_error("Failed to create stop event");
	}

	hCaptureEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (!hCaptureEvent) {
		throw std::runtime_error("Failed to create capture event");
	}

	hr = pAudioClient->SetEventHandle(hCaptureEvent);
	if (FAILED(hr)) {
		throw std::runtime_error("Failed to set capture event handle");
	}
}

OpusPlaybackServer::~OpusPlaybackServer() {
	if (clientSock != INVALID_SOCKET) closesocket(clientSock);
	if (listenSock != INVALID_SOCKET) closesocket(listenSock);
	WSACleanup();

	if (decoder) opus_decoder_destroy(decoder);
	if (pRenderClient) pRenderClient->Release();
	if (pAudioClient) pAudioClient->Release();
	if (pDevice) pDevice->Release();
	if (pEnumerator) pEnumerator->Release();

	if (hStopEvent) CloseHandle(hStopEvent);
	if (hCaptureEvent) CloseHandle(hCaptureEvent);
	if (hCaptureThread) CloseHandle(hCaptureThread);

	CoUninitialize();
}

bool OpusPlaybackServer::Start(uint16_t port) {
	listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSock == INVALID_SOCKET) return false;

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

	HRESULT hr = pAudioClient->Start();
	if (FAILED(hr)) {
		std::cerr << "Failed to start audio client: " << hr << std::endl;
		return false;
	}

	hCaptureThread = CreateThread(
		nullptr,
		0,
		CaptureThreadProc,
		this,
		0,
		nullptr
	);

	return true;
}

void OpusPlaybackServer::Stop() {
	SetEvent(hStopEvent);
	if (hCaptureThread) {
		WaitForSingleObject(hCaptureThread, INFINITE);
		CloseHandle(hCaptureThread);
		hCaptureThread = nullptr;
	}
	pAudioClient->Stop();
}

void OpusPlaybackServer::AcceptAndHandle() {
	clientSock = accept(listenSock, nullptr, nullptr);
	if (clientSock == INVALID_SOCKET) {
		return;
	}

	while (true) {
		// Receive data sizes of incoming data
		uint32_t dataSize;
		if (recv(clientSock, (char*)&dataSize, sizeof(dataSize), 0) != sizeof(dataSize)) {
			break;
		}

		// Receive the actual data
		std::vector<uint8_t> packet(dataSize);
		size_t totalReceived = 0;
		while (totalReceived < dataSize) {
			int received = recv(clientSock, (char*)packet.data() + totalReceived, dataSize - totalReceived, 0);
			if (received <= 0) {
				return;
			}
			totalReceived += received;
		}

		//std::cout << "Received " << packet.size() << " bytes of data over network. Queueing packet" << std::endl;
		std::lock_guard<std::mutex> lock(audioMutex);
		std::cout << "Queue size before push: " << audioQueue.size() << std::endl;
		audioQueue.push(std::move(packet));
	}
}

void OpusPlaybackServer::ConvertAndWriteAudio() {
	HRESULT hr;
	DWORD taskIndex = 0;

	HANDLE hTask = AvSetMmThreadCharacteristics(L"Audio", &taskIndex);
	if (hTask == nullptr) {
		throw std::runtime_error("Failed to set thread characteristics.");
	}

	while (true) {
		HANDLE waitHandles[] = { hCaptureEvent, hStopEvent };
		DWORD waitResult = WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);

		if (waitResult == WAIT_OBJECT_0 + 1) {  // Stop event signalled
			break;
		}
		else if (waitResult == WAIT_OBJECT_0) {
			for (int i = 0; i < 4; i++) {
				std::vector<uint8_t> packet;
				std::lock_guard<std::mutex> lock(audioMutex);
				if (audioQueue.empty()) break;
				packet = std::move(audioQueue.front());
				audioQueue.pop();

				if (!packet.empty()) {
					int samples = opus_decode_float(
						decoder,
						packet.data(),
						packet.size(),
						pcmInterleaved.data(),
						480,
						0
					);

					//std::cout << "Received " << samples << " samples, sending to output" << std::endl;

					if (samples > 0) {
						BYTE* pData;
						hr = pRenderClient->GetBuffer(samples, &pData);
						if (SUCCEEDED(hr)) {
							//std::cout << "First few samples: "
							//	<< pcmInterleaved[0] << ", "
							//	<< pcmInterleaved[1] << ", "
							//	<< pcmInterleaved[2] << std::endl;

							memcpy(pData, pcmInterleaved.data(), samples * 2 * sizeof(float));
							hr = pRenderClient->ReleaseBuffer(samples, 0);
							if (FAILED(hr)) {
								std::cerr << "Failed to release buffer" << std::endl;
							}
							else {
								//std::cout << "Successfully released " << samples << " frames to audio device" << std::endl;
							}
						}
					}
				}
			}
		}
	}
	AvRevertMmThreadCharacteristics(hTask);
}
