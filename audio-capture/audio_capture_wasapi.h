#pragma once
#define NOMINMAX
#include <Windows.h>
#include <stdio.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <fstream>

#include "mp3_encoder.h"

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "avrt.lib")

class AudioCapture {
public:
    AudioCapture();
    ~AudioCapture();

    bool Initialize();
    bool StartRecording(const std::string& filename, std::unique_ptr<IAudioEncoder> audioEncoder);
    void StopRecording();

private:
    void WriteWaveHeader();
    void CaptureThread();
    void CleanupCOM();

    // Device format info
    DWORD deviceSampleRate;
    WORD deviceChannels;
    WORD deviceBitsPerSample;

    // COM interfaces
    IMMDeviceEnumerator* pEnumerator;
    IMMDevice* pDevice;
    IAudioClient* pAudioClient;
    IAudioCaptureClient* pCaptureClient;

    std::unique_ptr<IAudioEncoder> encoder;
    std::ofstream encodedFile;

    HANDLE hCaptureThread;
    HANDLE hStopEvent;
    HANDLE hCaptureEvent;
    std::ofstream outputFile;
    UINT32 bufferFrames;
    DWORD dataSize;
    bool isRecording;
    bool isFloat;
    std::vector<float> floatBuffer;
};
