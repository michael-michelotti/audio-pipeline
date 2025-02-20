// main.cpp
#include <iostream>
#include <thread>
#include <chrono>

#include <media_pipeline/media_pipeline.h>
#include <muxing/ogg_muxer.h>
#include <muxing/mkv_muxer.h>

using namespace media_pipeline;

int main() {
    try {
        auto muxerQueue = std::make_shared<MediaQueue>();

        auto audio_source = std::make_shared<sources::audio::PortaudioSource>(44100, 1);
        //audio_source->Start();
        //while (true) {
        //    audio_source->GetMediaData();
        //}
        auto audio_processor = std::make_shared<processors::audio::Mp3Processor>();
        //auto file_format = std::make_unique<file_formats::Mp3FileFormat>();
        auto audio_sink = std::make_shared<sinks::general::MuxerSink>(muxerQueue);
        auto audio_pipeline = std::make_shared<MediaPipeline>(
            audio_source, 
            audio_processor, 
            audio_sink
        );

        auto video_source = std::make_shared<sources::video::WmfSource>();
        auto video_processor = std::make_shared<processors::video::HevcProcessor>();
        auto video_sink = std::make_shared<sinks::general::MuxerSink>(muxerQueue);
        auto video_pipeline = std::make_shared<MediaPipeline>(
            video_source,
            video_processor,
            video_sink
        );
        
        auto ogg_muxer = std::make_shared<MkvMuxer>(muxerQueue);
        std::cout << "Starting media muxer..." << std::endl;
        ogg_muxer->Start();

        std::cout << "Starting media pipelines..." << std::endl;
        audio_pipeline->Start();
        video_pipeline->Start();

        int recordTime = 10;
        std::cout << "Recording for " << recordTime << " seconds..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(recordTime));

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
