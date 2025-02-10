// main.cpp
#include <WinSock2.h>
#include <iostream>
#include <thread>
#include <chrono>
#include "audio_capture_wasapi.h"
#include "mp3_encoder.h"


int main() {
    try {
        AudioCapture capture;
        if (capture.Initialize()) {
            std::cout << "Starting audio capture..." << std::endl;

            // Create network-enabled encoder that will send data to server
            auto encoder = std::make_unique<NetworkMP3Encoder>(320);  // 320kbps quality

            if (capture.StartRecording("output.mp3", std::move(encoder))) {
                std::cout << "Recording and streaming to server. Press Enter to stop...\n";
                std::cin.get();
                capture.StopRecording();
                std::cout << "Recording stopped." << std::endl;
            }
            else {
                std::cout << "Error starting recording" << std::endl;
            }
        }
        else {
            std::cout << "Failed to initialize audio capture" << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
