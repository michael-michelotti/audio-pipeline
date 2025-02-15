#include "opus_processor.h"
#include <algorithm>
#include <iostream>

#define OPUS_LOGGING 0


OpusProcessor::OpusProcessor(int bitrate,
	int inputSampleRate,
	int channels,
	int frameSize) : 
	bitrate(bitrate), 
	inputSampleRate(inputSampleRate), 
	channels(channels), 
	frameSize(frameSize) {

	int error;
	encoder = opus_encoder_create(inputSampleRate, channels, OPUS_APPLICATION_AUDIO, &error);
	if (error != OPUS_OK || !encoder) {
		throw std::runtime_error("Failed to create Opus encoder");
	}

	opus_encoder_ctl(encoder, OPUS_SET_BITRATE(bitrate * 1000));
	opus_encoder_ctl(encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_MUSIC));
	opus_encoder_ctl(encoder, OPUS_SET_COMPLEXITY(10));
}

OpusProcessor::~OpusProcessor() {
	if (encoder) opus_encoder_destroy(encoder);
}

void OpusProcessor::Start() {
	// Not required
}

void OpusProcessor::Stop() {
	// Not required
}

MediaData OpusProcessor::ProcessMediaData(const MediaData& input) {
	const AudioFormat& inputFormat = input.getAudioFormat();
	size_t frameCount = input.data.size() / (inputFormat.channels * sizeof(float));

	if (OPUS_LOGGING) {
		std::cout << "<OpusProcessor> Received MediaData with " << frameCount << " frames." << std::endl;
	}

	// Allocate buffer for encoded data
	size_t maxOutputSize = static_cast<size_t>(1.25 * frameCount * inputFormat.channels + 7200);
	std::vector<uint8_t> encodedOpus;
	encodedOpus.resize(maxOutputSize);

	// Recast input data as pointer to PCM float data
	const float* inputBuffer = reinterpret_cast<const float*>(input.data.data());
	std::vector<float> frameBuffer(frameSize * channels, 0.0f);
	size_t pos = 0;
	size_t outputPos = 0;
	

	while (pos < frameCount) {
		size_t remainingFrames = frameCount - pos;
		size_t framesToEncode = remainingFrames > frameSize ? frameSize : remainingFrames;
		opus_int32 encodedBytes = 0;

		if (remainingFrames >= frameSize) {
			encodedBytes = opus_encode_float(
				encoder,
				inputBuffer + (pos * channels),
				frameSize,
				encodedOpus.data() + outputPos,
				encodedOpus.size() - outputPos
			);
		}
		else {
			std::memcpy(frameBuffer.data(), 
				inputBuffer + (pos * channels), 
				remainingFrames * channels * sizeof(float));

			encodedBytes = opus_encode_float(
				encoder,
				frameBuffer.data(),
				frameSize,
				encodedOpus.data() + outputPos,
				encodedOpus.size() - outputPos
			);
		}

		if (encodedBytes < 0) throw std::runtime_error("Opus encoding failed");
		outputPos += encodedBytes;
		pos += remainingFrames;
	}

	if (OPUS_LOGGING) {
		std::cout << "<OpusProcessor> Encoded " << frameCount << " frames into " << outputPos << " bytes." << std::endl;
	}

	encodedOpus.resize(outputPos);

	// Write frame count header
	std::vector<uint8_t> outputData;
	outputData.resize(sizeof(uint32_t) + encodedOpus.size());
	uint32_t inputFrames = static_cast<uint32_t>(frameCount);
	memcpy(outputData.data(), &inputFrames, sizeof(inputFrames));

	// Write Opus encoded data
	memcpy(outputData.data() + sizeof(uint32_t), encodedOpus.data(), outputPos);

	// Create output format for Opus data
	AudioFormat outputFormat;
	outputFormat.sampleRate = inputFormat.sampleRate;
	outputFormat.channels = inputFormat.channels;
	outputFormat.format = AudioFormat::SampleFormat::OPUS;

	if (OPUS_LOGGING) {
		std::cout << "<OpusProcessor> Outgoing MediaData statistics:"
			<< "\n Sample Rate: " << outputFormat.sampleRate
			<< "\n Channels: " << outputFormat.channels << std::endl;
	}

	MediaData output = MediaData::createAudio(outputData, outputFormat);
	return output;
}
