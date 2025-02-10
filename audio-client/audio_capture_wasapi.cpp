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
    , hCaptureEvent(nullptr)
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
        AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
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

    // Create capture event
    hCaptureEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!hCaptureEvent) {
        std::cerr << "Failed to create capture event." << std::endl;
        return false;
    }

    hr = pAudioClient->SetEventHandle(hCaptureEvent);
    if (FAILED(hr)) {
        std::cerr << "Failed to set capture event handle." << std::endl;
        return false;
    }

    return true;
}

bool AudioCapture::StartRecording(const std::string& filename, std::unique_ptr<IAudioEncoder> audioEncoder) {
    if (isRecording) return false;

    encoder = std::move(audioEncoder);
    if (!encoder->Initialize(deviceSampleRate, deviceChannels, deviceBitsPerSample)) {
        return false;
    }

    encodedFile.open(filename, std::ios::binary);
    if (!encodedFile.is_open()) return false;

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

    encodedFile.close();
    isRecording = false;
}

void AudioCapture::CaptureThread() {
    HRESULT hr;
    DWORD taskIndex = 0;
    BYTE* pData;
    UINT32 numFramesAvailable;
    UINT32 bytesToWrite;
    DWORD flags;
    HANDLE hTask = AvSetMmThreadCharacteristics(L"Audio", &taskIndex);

    if (hTask == nullptr) {
        std::cerr << "Failed to set thread characteristics." << std::endl;
    }

    while (isRecording) {
        // Wait indefinitely until new audio data is available or stop is requested
        HANDLE waitHandles[] = { hCaptureEvent, hStopEvent };
        DWORD waitResult = WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);

        if (waitResult == WAIT_OBJECT_0 + 1) {  // Stop event signaled
            break;
        }
        else if (waitResult == WAIT_OBJECT_0) {  // Capture event signaled
            HRESULT hr = pCaptureClient->GetNextPacketSize(&numFramesAvailable);

            if (SUCCEEDED(hr) && numFramesAvailable > 0) {
                hr = pCaptureClient->GetBuffer(
                    &pData,
                    &numFramesAvailable,
                    &flags,
                    nullptr,
                    nullptr
                );

                if (SUCCEEDED(hr)) {
                    float* stereoData = reinterpret_cast<float*>(pData);
                    UINT32 totalSamples  = numFramesAvailable * deviceChannels;

                    // Copy left sample to right sample to convert mono to stereo
                    for (UINT32 i = 0; i < numFramesAvailable; ++i) {
                        float leftSample = stereoData[i * 2];
                        float& rightSample = stereoData[i * 2 + 1];
                        rightSample = leftSample;
                    }

                    auto encodedData = encoder->Encode(stereoData, numFramesAvailable);
                    if (!encodedData.empty()) {
                        encodedFile.write(reinterpret_cast<const char*>(encodedData.data()), encodedData.size());
                    }

                    hr = pCaptureClient->ReleaseBuffer(numFramesAvailable);
                    if (FAILED(hr)) {
                        std::cerr << "Failed to release buffer." << std::endl;
                        break;
                    }
                }
            }
        }
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
    waveFormat.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    waveFormat.nChannels = deviceChannels;
    waveFormat.nSamplesPerSec = deviceSampleRate;
    waveFormat.wBitsPerSample = deviceBitsPerSample;
    waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
    waveFormat.nAvgBytesPerSec = deviceSampleRate * waveFormat.nBlockAlign;

    std::cout << "nChannels: " << waveFormat.nChannels << "\n"
        << "nBlockAlign: " << waveFormat.nBlockAlign << "\n"
        << "nAvgBytesPerSec: " << waveFormat.nAvgBytesPerSec << std::endl;

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
    if (hCaptureEvent) CloseHandle(hCaptureEvent);

    pCaptureClient = nullptr;
    pAudioClient = nullptr;
    pDevice = nullptr;
    pEnumerator = nullptr;
    hStopEvent = nullptr;
    hCaptureEvent = nullptr;
}
