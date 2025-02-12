#include "opus_processor.h"
#include <iostream>


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

AudioData OpusProcessor::ProcessAudioData(const AudioData& input) {
	AudioData output;
	output.frameCount = 0;

	if (input.frameCount == 0 || input.data.empty()) return output;

	const float* inputBuffer = reinterpret_cast<const float*>(input.data.data());
	size_t maxOutputSize = static_cast<size_t>(1.25 * input.frameCount * channels + 7200);
	output.data.resize(maxOutputSize);

	std::vector<float> frameBuffer(frameSize * channels, 0.0f);

	size_t pos = 0;
	size_t outputPos = 0;
	opus_int32 encodedBytes = 0;

	while (pos < input.frameCount) {
		size_t remainingFrames = input.frameCount - pos;

		if (remainingFrames > frameSize) {
			encodedBytes = opus_encode_float(
				encoder,
				inputBuffer + (pos * channels),
				frameSize,
				output.data.data() + outputPos,
				output.data.size() - outputPos
			);

			if (encodedBytes < 0) throw std::runtime_error("Opus encoding failed");
			outputPos += encodedBytes;
			pos += frameSize;
		}
		else {
			std::memcpy(frameBuffer.data(), inputBuffer + (pos * channels), remainingFrames * channels * sizeof(float));

			encodedBytes = opus_encode_float(
				encoder,
				frameBuffer.data(),
				frameSize,
				output.data.data() + outputPos,
				output.data.size() - outputPos
			);

			if (encodedBytes < 0) throw std::runtime_error("Opus encoding failed");
			outputPos += encodedBytes;
			pos += remainingFrames;
		}
	}

	output.data.resize(outputPos);
	std::cout << "pos: " << pos << ". Frame count: " << input.frameCount << std::endl;
	output.frameCount = pos;
	return output;
}
