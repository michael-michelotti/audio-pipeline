#include <stdexcept>
#include <iostream>

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wmcodecdsp.h>
#pragma comment(lib, "mfplat")
#pragma comment(lib, "mfuuid")
#pragma comment(lib, "mf")
#pragma comment(lib, "mfreadwrite")

#include "media_pipeline/sources/video/wmf_source.h"
#include "media_pipeline/core/interfaces/i_media_source.h"
#include "media_pipeline/core/interfaces/i_media_component.h"
#include "media_pipeline/core/media_data.h"

static int frameCount = 0;

namespace media_pipeline::sources::video {
	using core::MediaData;
	using core::VideoFormat;

	WmfSource::WmfSource() : pReader(nullptr), isInitialized(false) {
		HRESULT hr = MFStartup(MF_VERSION);
		if (FAILED(hr)) {
			throw std::runtime_error("Failed to initialize Media Foundation");
		}

		IMFAttributes* pConfig = nullptr;
		hr = MFCreateAttributes(&pConfig, 1);
		if (FAILED(hr)) {
			throw std::runtime_error("Failed to create MF attributes");
		}

		hr = pConfig->SetGUID(
			MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
			MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
		);
		if (FAILED(hr)) {
			throw std::runtime_error("Failed to find video capture device");
		}

		IMFActivate** ppDevices = nullptr;
		UINT32 deviceCount = 0;
		hr = MFEnumDeviceSources(pConfig, &ppDevices, &deviceCount);
		if (FAILED(hr)) {
			throw std::runtime_error("Failed to enumerate video capture devices");
		}

		if (deviceCount > 0) {
			IMFMediaSource* pSource = nullptr;
			hr = ppDevices[0]->ActivateObject(
				IID_PPV_ARGS(&pSource)
			);

			// Create reader for the source
			hr = MFCreateSourceReaderFromMediaSource(
				pSource,
				nullptr,
				&pReader
			);

			pSource->Release();
		}

		for (UINT32 i = 0; i < deviceCount; i++) {
			ppDevices[i]->Release();
		}

		IMFMediaType* pType = nullptr;
		hr = MFCreateMediaType(&pType);

		// Request 1920x1080 @ 30fps in YUY2 format
		pType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
		pType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_I420);
		MFSetAttributeSize(pType, MF_MT_FRAME_SIZE, 640, 480);
		MFSetAttributeRatio(pType, MF_MT_FRAME_RATE, 30, 1);

		// Try to set this format
		hr = pReader->SetCurrentMediaType(
			MF_SOURCE_READER_FIRST_VIDEO_STREAM,
			nullptr,
			pType
		);

		if (FAILED(hr)) {
			std::cout << "Failed to set requested format, falling back to default" << std::endl;
		}

		hr = pReader->GetCurrentMediaType(
			MF_SOURCE_READER_FIRST_VIDEO_STREAM,
			&pType
		);

		if (SUCCEEDED(hr)) {
			hr = MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, &width, &height);
			if (SUCCEEDED(hr)) {
				std::cout << "Frame size: " << width << "x" << height << std::endl;
			}

			UINT32 numerator, denominator;
			hr = MFGetAttributeRatio(pType, MF_MT_FRAME_RATE, &numerator, &denominator);
			if (SUCCEEDED(hr)) {
				frameRate = static_cast<float>(numerator) / denominator;
				std::cout << "Frame rate: " << frameRate << " fps" << std::endl;
			}

			// Get video format
			hr = pType->GetGUID(MF_MT_SUBTYPE, &subtype);
			if (SUCCEEDED(hr)) {
				if (subtype == MFVideoFormat_NV12) {
					std::cout << "Format: NV12" << std::endl;
				}
				else if (subtype == MFVideoFormat_YUY2) {
					std::cout << "Format: YUY2" << std::endl;
				}
				else if (subtype == MFVideoFormat_UYVY) {
					std::cout << "Format: UYVY" << std::endl;
				}
			}

			pType->Release();
		}

		CoTaskMemFree(ppDevices);
		pConfig->Release();
	}


	WmfSource::~WmfSource() {
		if (pReader) {
			pReader->Release();
		}
		MFShutdown();
	}

	void WmfSource::Start() {
		// Unnecessary, all done in constructor for now
	}


	void WmfSource::Stop() {
		// Unnecessary, reader released in destructor
	}

	MediaData WmfSource::GetMediaData() {
		if (!pReader) {
			throw std::runtime_error("Video reader not initialized");
		}

		MediaData data;
		DWORD streamIndex, flags;
		LONGLONG timestamp;
		IMFSample* pSample = nullptr;

		HRESULT hr = pReader->ReadSample(
			MF_SOURCE_READER_FIRST_VIDEO_STREAM,
			0,
			&streamIndex,
			&flags,
			&timestamp,
			&pSample
		);

		if (SUCCEEDED(hr) && pSample) {
			// Get buffer from sample
			IMFMediaBuffer* pBuffer = nullptr;
			hr = pSample->ConvertToContiguousBuffer(&pBuffer);

			if (SUCCEEDED(hr)) {
				BYTE* buffer = nullptr;
				DWORD bufferSize = 0;
				hr = pBuffer->Lock(&buffer, nullptr, &bufferSize);

				if (SUCCEEDED(hr)) {
					// Create video format
					VideoFormat format;
					format.width = width;
					format.height = height;
					format.format = VideoFormat::PixelFormat::YUV420P;
					format.frameRate = frameRate;

					// Copy data
					data.data.assign(buffer, buffer + bufferSize);
					data.type = MediaData::Type::Video;
					data.format = format;
					data.timestamp = timestamp;

					pBuffer->Unlock();
				}
				pBuffer->Release();
			}
			pSample->Release();
		}

		//std::cout << "Captured video data:"
		//	<< "\n Width: " << width
		//	<< "\n Height: " << height
		//	<< "\n Frame Rate: " << frameRate
		//	<< "\n Captured bytes: " << data.data.size() << std::endl;
		//	std::cout << "Format: " << (subtype == MFVideoFormat_I420 ? "I420" :
		//		subtype == MFVideoFormat_NV12 ? "NV12" : "Other") << std::endl;
		//	std::cout << "Total frames: " << frameCount++ << std::endl;
		return data;
	}
}
