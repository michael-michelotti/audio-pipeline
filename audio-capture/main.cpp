// main.cpp
#include "audio_capture.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    AudioCapture capture;

    if (capture.Initialize()) {
        std::cout << "Starting audio capture...\n";
        if (capture.StartRecording("output.wav")) {
            std::cout << "Recording for 5 seconds...\n";
            std::this_thread::sleep_for(std::chrono::seconds(5));
            capture.StopRecording();
            std::cout << "Recording stopped.\n";
        }
    }

    return 0;
}
