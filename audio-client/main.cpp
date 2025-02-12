// main.cpp
#include <WinSock2.h>
#include <iostream>
#include <thread>
#include <chrono>
#include "audio_pipeline.h"
#include "wasapi_default_input_source.h"
#include "mp3_processor.h"
#include "opus_processor.h"
#include "file_audio_sink.h"
#include "network_audio_sink.h"


int main() {
    try {
        auto source = std::make_shared<WasapiDefaultInputSource>();
        auto processor = std::make_shared<OpusProcessor>();
        auto oggFormat = std::make_unique<OggFileFormat>();
        auto sink = std::make_shared<NetworkAudioSink>("192.168.1.161", 12345);
        // auto sink = std::make_shared<FileAudioSink>("test.ogg", std::move(oggFormat));
        auto pipeline = std::make_shared<AudioPipeline>(source, processor, sink);

        std::cout << "Starting pipeline..." << std::endl;
        pipeline->Start();

        std::cout << "Recording for 5 seconds..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));

        std::cout << "Stopping pipeline..." << std::endl;
        pipeline->Stop();

        std::cout << "Clean shutdown complete" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
