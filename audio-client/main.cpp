// main.cpp
#include <WinSock2.h>
#include <iostream>
#include <thread>
#include <chrono>
#include "audio_pipeline.h"
#include "wasapi_default_input_source.h"
#include "mp3_processor.h"


int main() {
    std::shared_ptr<WasapiDefaultInputSource> source;
    std::shared_ptr<Mp3Processor> processor;
    std::shared_ptr<IAudioSink> sink;

    AudioPipeline* pipeline = new AudioPipeline(source, processor, sink);
    pipeline->Start();

    return 0;
}
