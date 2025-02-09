#include "audio_capture.h"


AudioCapture::AudioCapture() : hWaveIn(nullptr), dataSize(0), isRecording(false), outputFile() {
    // Initialize wave format structure
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = NUM_CHANNELS;
    waveFormat.nSamplesPerSec = SAMPLE_RATE;
    waveFormat.wBitsPerSample = BITS_PER_SAMPLE;
    waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
    waveFormat.cbSize = 0;
}

AudioCapture::~AudioCapture() {
    if (isRecording) {
        StopRecording();
    }
    CleanupBuffers();
}

bool AudioCapture::Initialize() {
    // Open wave input device
    MMRESULT result = waveInOpen(&hWaveIn, WAVE_MAPPER, &waveFormat,
        (DWORD_PTR)WaveInProc, (DWORD_PTR)this,
        CALLBACK_FUNCTION);
    if (result != MMSYSERR_NOERROR) {
        std::cerr << "Failed to open wave input device\n";
        return false;
    }

    // Allocate and prepare buffers
    for (int i = 0; i < NUM_BUFFERS; i++) {
        auto buffer = std::make_unique<BYTE[]>(BUFFER_SIZE);
        auto header = std::make_unique<WAVEHDR>();

        header->lpData = reinterpret_cast<LPSTR>(buffer.get());
        header->dwBufferLength = BUFFER_SIZE;
        header->dwBytesRecorded = 0;
        header->dwUser = 0;
        header->dwFlags = 0;

        result = waveInPrepareHeader(hWaveIn, header.get(), sizeof(WAVEHDR));
        if (result != MMSYSERR_NOERROR) {
            std::cerr << "Failed to prepare wave header\n";
            return false;
        }

        audioBuffers.push_back(std::move(buffer));
        waveHeaders.push_back(std::move(header));
    }

    return true;
}

bool AudioCapture::StartRecording(const std::string& filename) {
    outputFile.open(filename, std::ios::binary);
    if (!outputFile.is_open()) {
        std::cerr << "Failed to open output file\n";
        return false;
    }

    // Write WAV header (will be updated with correct size later)
    if (!WriteWaveHeader()) {
        return false;
    }

    dataSize = 0;
    isRecording = true;

    // Add buffers to recording queue
    for (auto& header : waveHeaders) {
        waveInAddBuffer(hWaveIn, header.get(), sizeof(WAVEHDR));
    }

    // Start recording
    MMRESULT result = waveInStart(hWaveIn);
    if (result != MMSYSERR_NOERROR) {
        std::cerr << "Failed to start recording\n";
        return false;
    }

    return true;
}

void AudioCapture::StopRecording() {
    if (!isRecording) return;

    isRecording = false;
    waveInStop(hWaveIn);
    waveInReset(hWaveIn);

    // Update WAV header with final size
    outputFile.seekp(0);
    WriteWaveHeader();

    outputFile.close();
}


void CALLBACK AudioCapture::WaveInProc(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance,
    DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
    if (uMsg == WIM_DATA) {
        AudioCapture* capture = reinterpret_cast<AudioCapture*>(dwInstance);
        WAVEHDR* pWaveHdr = reinterpret_cast<WAVEHDR*>(dwParam1);
        capture->ProcessWaveData(pWaveHdr);
    }
}

void AudioCapture::ProcessWaveData(WAVEHDR* pWaveHdr) {
    if (!isRecording) return;

    // Write recorded data to file
    outputFile.write(pWaveHdr->lpData, pWaveHdr->dwBytesRecorded);
    dataSize += pWaveHdr->dwBytesRecorded;

    // Requeue the buffer for more recording
    if (isRecording) {
        pWaveHdr->dwBytesRecorded = 0;
        waveInAddBuffer(hWaveIn, pWaveHdr, sizeof(WAVEHDR));
    }
}

bool AudioCapture::WriteWaveHeader() {
    // RIFF header
    outputFile.write("RIFF", 4);
    DWORD fileSize = dataSize + 36; // Size of entire file - 8
    outputFile.write(reinterpret_cast<const char*>(&fileSize), 4);
    outputFile.write("WAVE", 4);

    // Format chunk
    outputFile.write("fmt ", 4);
    DWORD fmtSize = 16; // Size of format chunk
    outputFile.write(reinterpret_cast<const char*>(&fmtSize), 4);
    outputFile.write(reinterpret_cast<const char*>(&waveFormat), 16);

    // Data chunk
    outputFile.write("data", 4);
    outputFile.write(reinterpret_cast<const char*>(&dataSize), 4);

    return outputFile.good();
}

void AudioCapture::CleanupBuffers() {
    if (hWaveIn) {
        for (auto& header : waveHeaders) {
            if (header->dwFlags & WHDR_PREPARED) {
                waveInUnprepareHeader(hWaveIn, header.get(), sizeof(WAVEHDR));
            }
        }
        waveInClose(hWaveIn);
        hWaveIn = nullptr;
    }

    waveHeaders.clear();
    audioBuffers.clear();
}
