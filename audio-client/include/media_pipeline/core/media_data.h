#pragma once
#include <vector>
#include <variant>

namespace media_pipeline::core {
	struct AudioFormat {
		int sampleRate;
		int channels;
		enum class SampleFormat {
			PCM_S16LE,
			PCM_S24LE,
			PCM_S32LE,
			PCM_FLOAT,
			OPUS,
			MP3,
			AAC
		} format;

		int bitDepth;
		int bytesPerFrame;
	};

	struct VideoFormat {
		int width;
		int height;
		enum class PixelFormat {
			YUV420P,
			RGB24,
			BGR24,
			THEORA,
			THEORA_HEADERS,
			HEVC,
			HEVC_HEADERS
		} format;

		float frameRate;
		bool isKeyFrame;
		uint64_t granulepos = 0;
	};

	struct MediaData {
		std::vector<uint8_t> data;
		uint64_t timestamp;
		bool isEndOfStream = false;
		enum class Type { Audio, Video } type;
		std::variant<AudioFormat, VideoFormat> format;

		AudioFormat& getAudioFormat() {
			return std::get<AudioFormat>(format);
		}

		const AudioFormat& getAudioFormat() const {
			return std::get<AudioFormat>(format);
		}

		VideoFormat& getVideoFormat() {
			return std::get<VideoFormat>(format);
		}

		const VideoFormat& getVideoFormat() const {
			return std::get<VideoFormat>(format);
		}

		static MediaData createAudio(std::vector<uint8_t> data, AudioFormat format) {
			MediaData md;
			md.data = std::move(data);
			md.type = Type::Audio;
			md.format = std::move(format);
			return md;
		}

		static MediaData createVideo(std::vector<uint8_t> data, VideoFormat format) {
			MediaData md;
			md.data = std::move(data);
			md.type = Type::Video;
			md.format = std::move(format);
			return md;
		}
	};
}
