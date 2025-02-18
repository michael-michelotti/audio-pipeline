# Media Pipeline Library

A modular C++ library for building audio and video processing pipelines. Supports various input sources, processors, and output sinks, with the ability to mux multiple streams.

## Features

- **Multiple Input Sources**
  - Audio: WASAPI, PortAudio
  - Video: Windows Media Foundation

- **Audio/Video Processing**
  - Audio: MP3, Opus encoding
  - Video: Theora encoding

- **Flexible Output Options**
  - File output (MP3, OGG)
  - Network streaming
  - Multi-stream muxing

## Project Structure

```
include/                        # Public headers
├── media_pipeline/            # Main library headers
│   ├── core/                  # Core components
│   │   ├── interfaces/        # Abstract interfaces
│   │   ├── media_data.h      # Data structures
│   │   ├── media_queue.h     # Queue implementation
│   │   └── pipeline.h        # Pipeline implementation
│   ├── sources/              # Input sources
│   │   ├── audio/           
│   │   └── video/
│   ├── processors/           # Media processors
│   │   ├── audio/
│   │   └── video/
│   ├── sinks/               # Output sinks
│   │   ├── general/         # Format-agnostic sinks
│   │   ├── audio/          
│   │   └── video/
│   └── file_formats/        # File format handlers
└── muxing/                   # Multi-stream muxing
```

## Namespace Organization

The library uses namespaces that mirror the directory structure:

```cpp
namespace media_pipeline {
    namespace core {          // Core components
        namespace interfaces {} // Abstract interfaces
    }
    namespace sources {       // Input sources
        namespace audio {}
        namespace video {}
    }
    namespace processors {    // Media processors
        namespace audio {}
        namespace video {}
    }
    namespace sinks {        // Output sinks
        namespace general {}
        namespace audio {}
        namespace video {}
    }
    namespace file_formats {} // File format handlers
}

namespace muxing {}          // Multi-stream muxing
```

## Basic Usage

Here's a simple example that creates an audio pipeline that records from a WASAPI source and saves to an MP3 file:

```cpp
#include <media_pipeline/media_pipeline.h>

using namespace media_pipeline;

int main() {
    // Create components
    auto source = std::make_shared<sources::audio::WasapiSource>();
    auto processor = std::make_shared<processors::audio::Mp3Processor>();
    auto sink = std::make_shared<sinks::general::FileSink>(
        std::make_shared<file_formats::Mp3Format>()
    );

    // Create and start pipeline
    auto pipeline = std::make_shared<MediaPipeline>(source, processor, sink);
    pipeline->Start();

    // Record for 10 seconds
    std::this_thread::sleep_for(std::chrono::seconds(10));

    // Stop pipeline
    pipeline->Stop();
    return 0;
}
```

## Building the Project

### Prerequisites
- Visual Studio 2019 or later
- Windows SDK
- PortAudio SDK (optional)
- Opus SDK (optional)
- MP3Lame SDK (optional)

### Building with Visual Studio
1. Open the solution file
2. Set your platform and configuration (Debug/Release, x64/x86)
3. Build the solution

## Dependencies

- **Windows Media Foundation**: Video capture
- **WASAPI**: Windows audio capture
- **PortAudio**: Cross-platform audio capture
- **Opus**: Audio encoding
- **MP3Lame**: MP3 encoding
- **Theora**: Video encoding

## Contributing

1. Follow the existing namespace and directory structure
2. Place headers in include/ and implementations in src/
3. Maintain consistent naming conventions:
   - Classes: PascalCase
   - Functions: camelCase
   - Files: snake_case
4. Add appropriate unit tests for new components
