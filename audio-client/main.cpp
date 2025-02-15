// main.cpp
#include <WinSock2.h>
#include <iostream>
#include <thread>
#include <chrono>
#include "media_pipeline.h"
#include "wasapi_audio_input_source.h"
#include "portaudio_input_source.h"
#include "mp3_processor.h"
#include "opus_processor.h"
#include "file_audio_sink.h"
#include "network_audio_sink.h"
#include "muxer_audio_sink.h"
#include "wmf_video_input_source.h"
#include "theora_processor.h"

#include "muxer.h"
#include "media_queue.h"

int main() {
    try {
        auto muxerQueue = std::make_shared<MediaQueue>();

        auto audio_source = std::make_shared<WasapiAudioInputSource>();
        auto audio_processor = std::make_shared<OpusProcessor>();
        auto audio_sink = std::make_shared<MuxerAudioSink>(muxerQueue);
        auto audio_pipeline = std::make_shared<MediaPipeline>(
            audio_source, 
            audio_processor, 
            audio_sink
        );

        auto video_source = std::make_shared<WebcamSource>();
        auto video_processor = std::make_shared<TheoraProcessor>();
        auto video_sink = std::make_shared<MuxerAudioSink>(muxerQueue);
        auto video_pipeline = std::make_shared<MediaPipeline>(
            video_source,
            video_processor,
            video_sink
        );
        

        auto ogg_muxer = std::make_shared<OggMuxer>(muxerQueue);
        std::cout << "Starting media muxer..." << std::endl;
        ogg_muxer->Start();

        std::cout << "Starting media pipelines..." << std::endl;
        audio_pipeline->Start();
        video_pipeline->Start();

        std::cout << "Recording for 5 seconds..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(10));

        std::cout << "Stopping media muxer..." << std::endl;
        ogg_muxer->Stop();

        std::cout << "Stopping media pipelines..." << std::endl;
        audio_pipeline->Stop();
        video_pipeline->Stop();

        std::cout << "Clean shutdown complete" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
