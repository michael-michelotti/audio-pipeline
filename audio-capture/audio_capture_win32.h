#pragma once

#include <Windows.h>
#include <stdio.h>
#include <mmsystem.h>
#include <mmdeviceapi.h>
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <fstream>

#pragma comment(lib, "winmm.lib")

// audio_capture.h
class AudioCaptureWin32 {
public:
    AudioCaptureWin32();
    ~AudioCaptureWin32();

    bool Initialize();
    bool StartRecording(const std::string& filename);
    void StopRecording();

private:
    static void CALLBACK WaveInProc(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance,
        DWORD_PTR dwParam1, DWORD_PTR dwParam2);

    void ProcessWaveData(WAVEHDR* pWaveHdr);
    bool WriteWaveHeader();
    void CleanupBuffers();

    static constexpr int SAMPLE_RATE = 44100;
    static constexpr int BITS_PER_SAMPLE = 16;
    static constexpr int NUM_CHANNELS = 1;
    static constexpr int BUFFER_SIZE = 8192;
    static constexpr int NUM_BUFFERS = 3;

    WAVEFORMATEX waveFormat;
    HWAVEIN hWaveIn;
    std::ofstream outputFile;
    std::vector<std::unique_ptr<WAVEHDR>> waveHeaders;
    std::vector<std::unique_ptr<BYTE[]>> audioBuffers;
    DWORD dataSize;
    bool isRecording;
};
