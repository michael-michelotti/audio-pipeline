#include <stdexcept>
#include <iostream>
#include <algorithm>

#include <lame/lame.h>

#include "media_pipeline/processors/audio/mp3_processor.h"
#include "media_pipeline/core/interfaces/i_media_processor.h"
#include "media_pipeline/core/interfaces/i_media_component.h"
#include "media_pipeline/core/media_data.h"

namespace media_pipeline::processors::audio {
	using core::MediaData;
	using core::AudioFormat;

	Mp3Processor::Mp3Processor(int requestedBitrate)
		: lameFlags(nullptr)
		, isInitialized(false)
		, bitrate(requestedBitrate)
		, outputSampleRate(44100)
		, inputSampleRate(0)
		, channels(0) { }

	Mp3Processor::~Mp3Processor() {
		if (lameFlags) lame_close(lameFlags);
	}

	void Mp3Processor::Start() {
		// Nothing required
	}

	void Mp3Processor::Stop() {
		// Nothing required
	}

	MediaData Mp3Processor::ProcessMediaData(const MediaData& input) {
		if (!isInitialized) {
			// First frame of audio, use parameters to configure encoder
			if (!InitializeEncoder(input)) {
				throw std::runtime_error("Failed to initialize MP3 encoder");
			}
		}

		MediaData output;
		const AudioFormat& inputFormat = input.getAudioFormat();

		size_t bytesPerSample;
		switch (inputFormat.format) {
		case AudioFormat::SampleFormat::PCM_S16LE:
			bytesPerSample = 2;
			break;
		case AudioFormat::SampleFormat::PCM_FLOAT:
		case AudioFormat::SampleFormat::PCM_S32LE:
		case AudioFormat::SampleFormat::PCM_S24LE:
			bytesPerSample = 4;
			break;
		default:
			throw std::runtime_error("Unsupported format on audio input device");
		}

		size_t frameCount = input.data.size() / (bytesPerSample * inputFormat.channels);
		size_t maxOutputSize = static_cast<size_t>(1.25 * frameCount * bytesPerSample * inputFormat.channels + 7200);
		output.data.resize(maxOutputSize);

		const short* inputData = reinterpret_cast<const short*>(input.data.data());
		std::vector<short> scaledBuffer(frameCount);
		int encodedBytes;
		switch (inputFormat.format) {
		case AudioFormat::SampleFormat::PCM_FLOAT:
			encodedBytes = lame_encode_buffer_interleaved_ieee_float(
				lameFlags,
				reinterpret_cast<const float*>(input.data.data()),
				frameCount * inputFormat.channels,
				output.data.data(),
				output.data.size()
			);
			break;
		case AudioFormat::SampleFormat::PCM_S16LE:
			encodedBytes = lame_encode_buffer(
				lameFlags,
				const_cast<short*>(reinterpret_cast<const short*>(input.data.data())),
				nullptr,
				frameCount,
				output.data.data(),
				output.data.size()
			);

			break;
		default:
			encodedBytes = -1;
		}

		if (encodedBytes < 0) {
			throw std::runtime_error("MP3 encoding failed");
		}

		output.data.resize(encodedBytes);
		AudioFormat outputFormat = inputFormat;
		outputFormat.format = AudioFormat::SampleFormat::MP3;
		outputFormat.channels = lame_get_mode(lameFlags) == MONO ? 1 : 2;
		output.format = outputFormat;
		return output;
	}

	bool Mp3Processor::InitializeEncoder(const MediaData& firstPacket) {
		lameFlags = lame_init();
		if (!lameFlags) return false;

		const AudioFormat& inputFormat = firstPacket.getAudioFormat();

		lame_set_in_samplerate(lameFlags, inputFormat.sampleRate);
		lame_set_out_samplerate(lameFlags, outputSampleRate);
		lame_set_num_channels(lameFlags, inputFormat.channels);
		switch (inputFormat.channels) {
		case 1:
			lame_set_mode(lameFlags, MONO);
			break;
		case 2:
			lame_set_mode(lameFlags, STEREO);
			break;
		}
		lame_set_brate(lameFlags, bitrate);
		lame_set_quality(lameFlags, 2);
		lame_set_VBR(lameFlags, vbr_off);

		if (lame_init_params(lameFlags) < 0) {
			lame_close(lameFlags);
			lameFlags = nullptr;
			return false;
		}

		std::cout << "LAME Configuration:" << std::endl;
		std::cout << "Input Samplerate: " << lame_get_in_samplerate(lameFlags) << std::endl;
		std::cout << "Output Samplerate: " << lame_get_out_samplerate(lameFlags) << std::endl;
		std::cout << "Number of channels: " << lame_get_num_channels(lameFlags) << std::endl;
		std::cout << "Mode: " << lame_get_mode(lameFlags) << std::endl;
		std::cout << "Quality level: " << lame_get_quality(lameFlags) << std::endl;
		std::cout << "Bitrate: " << lame_get_brate(lameFlags) << " kbps" << std::endl;
		std::cout << "VBR mode: " << lame_get_VBR(lameFlags) << std::endl;
		std::cout << "Scale: " << lame_get_scale(lameFlags) << std::endl;
		std::cout << "Force MS: " << lame_get_force_ms(lameFlags) << std::endl;
		std::cout << "Compression ratio: " << lame_get_compression_ratio(lameFlags) << std::endl;

		isInitialized = true;
		return true;
	}
}
