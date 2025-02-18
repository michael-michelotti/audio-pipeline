#pragma once

namespace media_pipeline::core::interfaces {
	class IMediaComponent {
	public:
		virtual ~IMediaComponent() = default;
		virtual void Start() = 0;
		virtual void Stop() = 0;
	};
}
