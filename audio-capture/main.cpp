// main.cpp
#include <iostream>
#include <thread>
#include <chrono>
#include <lame/lame.h>

#include "audio_capture_wasapi.h"
#include "mp3_encoder.h"


int main() {
    AudioCapture capture;
    auto encoder = std::make_unique<MP3Encoder>(320);

    if (capture.Initialize()) {
        std::cout << "Starting audio capture..." << std::endl;
        if (capture.StartRecording("output.mp3", std::move(encoder))) {
            std::cout << "Recording for 5 seconds..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
            capture.StopRecording();
            std::cout << "Recording stopped." << std::endl;
        }
        else {
            std::cout << "Error opening audio capture" << std::endl;
        }
    }

    return 0;
}
