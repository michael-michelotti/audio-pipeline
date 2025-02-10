#include "audio_capture_wasapi.h"
#include <functiondiscoverykeys_devpkey.h>
#include <avrt.h>
#include <thread>


AudioCapture::AudioCapture()
    : pEnumerator(nullptr)
    , pDevice(nullptr)
    , pAudioClient(nullptr)
    , pCaptureClient(nullptr)
    , hCaptureThread(nullptr)
    , hStopEvent(nullptr)
    , outputFile()
    , bufferFrames(0)
    , dataSize(0)
    , isRecording(false)
    , isFloat(false)
    , deviceSampleRate(0)
    , deviceChannels(0)
    , deviceBitsPerSample(0)
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to initialize COM");
    }
}

AudioCapture::~AudioCapture() {
    if (isRecording) {
        StopRecording();
    }
    CleanupCOM();
    CoUninitialize();
}

bool AudioCapture::Initialize() {
    HRESULT hr;

    // Create device enumerator
    hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        nullptr,
        CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator),
        (void**)&pEnumerator
    );
    if (FAILED(hr)) {
        std::cerr << "Failed to create device enumerator." << std::endl;
        return false;
    }

    // Get default input device
    hr = pEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &pDevice);
    if (FAILED(hr)) {
        std::cerr << "Failed to get default audio endpoint." << std::endl;
        return false;
    }

    // Create audio client
    hr = pDevice->Activate(
        __uuidof(IAudioClient),
        CLSCTX_ALL,
        nullptr,
        (void**)&pAudioClient
    );
    if (FAILED(hr)) {
        std::cerr << "Failed to activate audio client." << std::endl;
        return false;
    }

    // Get the device's mix format
    WAVEFORMATEX* deviceFormat = nullptr;
    hr = pAudioClient->GetMixFormat(&deviceFormat);
    if (FAILED(hr)) {
        std::cerr << "Failed to get device mix format." << std::endl;
        return false;
    }

    // Store device format details
    deviceSampleRate = deviceFormat->nSamplesPerSec;
    deviceChannels = deviceFormat->nChannels;
    deviceBitsPerSample = deviceFormat->wBitsPerSample;

    // Check if format is float
    isFloat = (deviceFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) ||
        (deviceFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
            reinterpret_cast<WAVEFORMATEXTENSIBLE*>(deviceFormat)->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);

    std::cout << "Device format:"
        << "\n Sample rate: " << deviceSampleRate
        << "\n Channels: " << deviceChannels
        << "\n Bits per sample: " << deviceBitsPerSample
        << "\n Is Float: " << (isFloat ? "Yes" : "No") << std::endl;

    // Initialize audio client
    hr = pAudioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        0,
        0,
        0,
        deviceFormat,
        nullptr);

    CoTaskMemFree(deviceFormat);

    if (FAILED(hr)) {
        std::cerr << "Failed to initialize audio client. Error: 0x"
            << std::hex << hr << std::endl;
        return false;
    }

    // Get buffer size
    hr = pAudioClient->GetBufferSize(&bufferFrames);
    if (FAILED(hr)) {
        std::cerr << "Failed to get buffer size." << std::endl;
        return false;
    }

    // Get capture client
    hr = pAudioClient->GetService(
        __uuidof(IAudioCaptureClient),
        (void**)&pCaptureClient
    );
    if (FAILED(hr)) {
        std::cerr << "Failed to get capture client." << std::endl;
        return false;
    }

    // Create stop event
    hStopEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (hStopEvent == nullptr) {
        std::cerr << "Failed to create stop event." << std::endl;
        return false;
    }

    // Allocate float buffer if needed
    if (isFloat) {
        floatBuffer.resize(bufferFrames * deviceChannels);
    }

    return true;
}

bool AudioCapture::StartRecording(const std::string& filename) {
    if (isRecording) return false;

    if (!pAudioClient || !pCaptureClient) {
        std::cerr << "Audio interfaces not properly initialized" << std::endl;
        return false;
    }

    outputFile.open(filename, std::ios::binary);
    if (!outputFile.is_open()) return false;

    WriteWaveHeader();
    dataSize = 0;
    isRecording = true;

    // Start audio client
    HRESULT hr = pAudioClient->Start();
    if (FAILED(hr)) return false;

    // Create capture thread
    hCaptureThread = CreateThread(
        nullptr,
        0,
        [](LPVOID param) -> DWORD {
            auto capture = static_cast<AudioCapture*>(param);
            capture->CaptureThread();
            return 0;
        },
        this,
        0,
        nullptr
    );

    return true;
}

void AudioCapture::StopRecording() {
    if (!isRecording) return;

    // Signal thread to stop
    SetEvent(hStopEvent);

    // Wait for thread to finish
    if (hCaptureThread) {
        WaitForSingleObject(hCaptureThread, INFINITE);
        CloseHandle(hCaptureThread);
        hCaptureThread = nullptr;
    }

    // Stop audio client
    pAudioClient->Stop();

    // Update WAV header with final size
    outputFile.seekp(4); // Move to file size position
    DWORD fileSize = dataSize + 36;
    outputFile.write(reinterpret_cast<const char*>(&fileSize), 4);

    outputFile.seekp(40); // Move to data size position
    outputFile.write(reinterpret_cast<const char*>(&dataSize), 4);
    // outputFile.seekp(0);
    WriteWaveHeader();

    outputFile.close();
    isRecording = false;
}

void AudioCapture::CaptureThread() {
    HRESULT hr;
    DWORD taskIndex = 0;
    HANDLE hTask = AvSetMmThreadCharacteristics(L"Audio", &taskIndex);
    if (hTask == nullptr) {
        std::cerr << "Failed to set thread characteristics." << std::endl;
    }

    BYTE* pData;
    UINT32 numFramesAvailable;
    DWORD flags;

    // Get device period for proper sleep timing
    REFERENCE_TIME hnsDefaultDevicePeriod;
    pAudioClient->GetDevicePeriod(&hnsDefaultDevicePeriod, NULL);
    DWORD sleepTime = static_cast<DWORD>(hnsDefaultDevicePeriod / 10000); // Convert to milliseconds

    while (isRecording) {
        if (WaitForSingleObject(hStopEvent, 0) == WAIT_OBJECT_0) {
            break;
        }

        hr = pCaptureClient->GetNextPacketSize(&numFramesAvailable);
        if (FAILED(hr)) {
            std::cerr << "Failed to get next packet size." << std::endl;
            break;
        }

        if (numFramesAvailable > 0) {
            hr = pCaptureClient->GetBuffer(
                &pData,
                &numFramesAvailable,
                &flags,
                nullptr,
                nullptr
            );

            if (SUCCEEDED(hr)) {
                if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT)) {
                    if (isFloat) {
                        // Convert float samples to 16-bit PCM
                        float* floatData = reinterpret_cast<float*>(pData);
                        std::vector<short> pcmData(numFramesAvailable * deviceChannels);

                        for (UINT32 i = 0; i < numFramesAvailable * deviceChannels; i++) {
                            float sample = floatData[i];
                            // Clamp to [-1, 1] and convert to 16-bit
                            sample = std::max(-1.0f, std::min(1.0f, sample));
                            pcmData[i] = static_cast<short>(sample * 32767.0f);
                        }

                        // Write converted data
                        outputFile.write(reinterpret_cast<const char*>(pcmData.data()),
                            numFramesAvailable * deviceChannels * sizeof(short));
                        dataSize += numFramesAvailable * deviceChannels * sizeof(short);
                    }
                    else {
                        // Write raw PCM data
                        UINT32 bytesToWrite = numFramesAvailable * deviceChannels * sizeof(float);
                        outputFile.write(reinterpret_cast<char*>(pData), bytesToWrite);
                        dataSize += bytesToWrite;
                    }
                }

                hr = pCaptureClient->ReleaseBuffer(numFramesAvailable);
                if (FAILED(hr)) {
                    std::cerr << "Failed to release buffer." << std::endl;
                    break;
                }
            }
        }

        Sleep(sleepTime);
    }

    if (hTask) {
        AvRevertMmThreadCharacteristics(hTask);
    }
}

void AudioCapture::WriteWaveHeader() {
    // RIFF header
    outputFile.write("RIFF", 4);
    DWORD fileSize = dataSize + 36; // Size of entire file - 8
    outputFile.write(reinterpret_cast<const char*>(&fileSize), 4);
    outputFile.write("WAVE", 4);

    // Format chunk
    outputFile.write("fmt ", 4);
    DWORD fmtSize = 16;
    outputFile.write(reinterpret_cast<const char*>(&fmtSize), 4);

    // Always write 16-bit PCM format
    WAVEFORMATEX waveFormat = {};
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = WAV_CHANNELS;
    waveFormat.nSamplesPerSec = WAV_SAMPLE_RATE;
    waveFormat.wBitsPerSample = WAV_BITS_PER_SAMPLE;
    waveFormat.nBlockAlign = (WAV_CHANNELS * WAV_BITS_PER_SAMPLE) / 8;
    waveFormat.nAvgBytesPerSec = WAV_SAMPLE_RATE * waveFormat.nBlockAlign;

    outputFile.write(reinterpret_cast<const char*>(&waveFormat), 16);

    // Data chunk
    outputFile.write("data", 4);
    outputFile.write(reinterpret_cast<const char*>(&dataSize), 4);
}

void AudioCapture::CleanupCOM() {
    if (pCaptureClient) pCaptureClient->Release();
    if (pAudioClient) pAudioClient->Release();
    if (pDevice) pDevice->Release();
    if (pEnumerator) pEnumerator->Release();
    if (hStopEvent) CloseHandle(hStopEvent);

    pCaptureClient = nullptr;
    pAudioClient = nullptr;
    pDevice = nullptr;
    pEnumerator = nullptr;
    hStopEvent = nullptr;
}
