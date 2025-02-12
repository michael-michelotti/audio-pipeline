#include "mp3_processor.h"


Mp3Processor::Mp3Processor(
	int bitrate, 
	int outputSampleRate,
	int inputSampleRate,
	int channels)
	: bitrate(bitrate)
	, outputSampleRate(outputSampleRate)
	, inputSampleRate(inputSampleRate)
	, channels(channels) {

	lameFlags = lame_init();
	if (!lameFlags) throw std::runtime_error("Failed to initialize LAME encoder");

	lame_set_in_samplerate(lameFlags, inputSampleRate);
	lame_set_out_samplerate(lameFlags, outputSampleRate);
	lame_set_num_channels(lameFlags, channels);
	lame_set_brate(lameFlags, bitrate);
	lame_set_quality(lameFlags, 2);
	lame_set_VBR(lameFlags, vbr_off);

	if (lame_init_params(lameFlags) < 0) {
		lame_close(lameFlags);
		lameFlags = nullptr;
		throw std::runtime_error("Failed to initialize LAME encoder");
	}
}

Mp3Processor::~Mp3Processor() {
	if (lameFlags) lame_close(lameFlags);
}

void Mp3Processor::Start() {
	// Nothing required
}

void Mp3Processor::Stop() {
	// Nothing required
}

AudioData Mp3Processor::ProcessAudioData(const AudioData& input) {
	AudioData output;

	const float* inputBuffer = reinterpret_cast<const float*>(input.data.data());
	size_t numSamples = input.sampleCount * channels;

	int encodedBytes = lame_encode_buffer_interleaved_ieee_float(
		lameFlags,
		inputBuffer,
		input.sampleCount,
		output.data.data(),
		output.data.size()
	);

	if (encodedBytes < 0) throw std::runtime_error("MP3 encoding failed");

	output.data.resize(encodedBytes);
	output.sampleCount = input.sampleCount;
	return output;
}
