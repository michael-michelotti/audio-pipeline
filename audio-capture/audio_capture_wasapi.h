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

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "avrt.lib")

class AudioCapture {
public:
    AudioCapture();
    ~AudioCapture();

    bool Initialize();
    bool StartRecording(const std::string& filename);
    void StopRecording();

private:
    void WriteWaveHeader();
    void CaptureThread();
    void CleanupCOM();

    // Device format info
    DWORD deviceSampleRate;
    WORD deviceChannels;
    WORD deviceBitsPerSample;

    // Output WAV format
    static constexpr WORD WAV_CHANNELS = 2;
    static constexpr WORD WAV_BITS_PER_SAMPLE = 16;
    static constexpr DWORD WAV_SAMPLE_RATE = 48000;

    // COM interfaces
    IMMDeviceEnumerator* pEnumerator;
    IMMDevice* pDevice;
    IAudioClient* pAudioClient;
    IAudioCaptureClient* pCaptureClient;

    HANDLE hCaptureThread;
    HANDLE hStopEvent;
    std::ofstream outputFile;
    UINT32 bufferFrames;
    DWORD dataSize;
    bool isRecording;
    bool isFloat;
    std::vector<float> floatBuffer;
};
