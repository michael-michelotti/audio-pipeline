#include "opus/opus.h"
#include "opus_processor.h"


OpusProcessor::OpusProcessor(int bitrate,
	int inputSampleRate,
	int channels) : bitrate(bitrate), inputSampleRate(inputSampleRate), channels(channels) {

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
	output.sampleCount = 0;

	if (input.sampleCount == 0 || input.data.empty()) return output;

	const float* inputBuffer = reinterpret_cast<const float*>(input.data.data());
	size_t maxOutputSize = static_cast<size_t>(1.25 * input.sampleCount * channels + 7200);
	output.data.resize(maxOutputSize);

	size_t pos = 0;
	size_t outputPos = 0;

	while (pos < input.sampleCount) {
		opus_int32 encodedBytes = opus_encode_float(
			encoder,
			inputBuffer + (pos * channels),
			960,
			output.data.data() + outputPos,
			output.data.size() - outputPos
		);

		if (encodedBytes < 0) throw std::runtime_error("Opus encoding failed");

		outputPos += encodedBytes;
		pos += 960;
	}

	output.data.resize(outputPos);
	output.sampleCount = input.sampleCount;
	return output;
}
