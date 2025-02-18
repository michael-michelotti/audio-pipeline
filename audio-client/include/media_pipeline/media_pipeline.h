#pragma once
/******** The main include file for utilization of the MediaPipeline class *********/

// ----- Core interfaces -----
#include "core/interfaces/i_media_component.h"
#include "core/interfaces/i_media_source.h"
#include "core/interfaces/i_media_processor.h"
#include "core/interfaces/i_media_sink.h"
#include "core/interfaces/i_file_format.h"
#include "core/media_data.h"
#include "core/media_queue.h"
#include "core/pipeline.h"

// ----- Public components -----
// File Formats
#include "file_formats/mp3_format.h"
#include "file_formats/ogg_format.h"

// Processors
#include "processors/audio/mp3_processor.h"
#include "processors/audio/opus_processor.h"
#include "processors/video/theora_processor.h"

// Sources
#include "sources/audio/portaudio_source.h"
#include "sources/audio/wasapi_source.h"
#include "sources/video/wmf_source.h"

// Sinks
#include "sinks/general/file_sink.h"
#include "sinks/general/muxer_sink.h"
#include "sinks/general/network_sink.h"

namespace media_pipeline {
	using core::interfaces::IFileFormat;
	using core::interfaces::IMediaComponent;
	using core::interfaces::IMediaProcessor;
	using core::interfaces::IMediaSink;
	using core::interfaces::IMediaSource;

	using core::AudioFormat;
	using core::VideoFormat;
	using core::MediaData;
	using core::MediaPipeline;
	using core::MediaQueue;
}
