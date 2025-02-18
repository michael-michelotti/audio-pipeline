#pragma once

class IMediaComponent {
public:
	virtual ~IMediaComponent() = default;
	virtual void Start() = 0;
	virtual void Stop() = 0;
};
