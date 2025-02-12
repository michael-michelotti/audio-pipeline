#include "file_audio_sink.h"
#include <ogg/ogg.h>
#include <iostream>


// ----- MP3 FILE FORMAT -----
void Mp3FileFormat::WriteHeader(std::ofstream& file) {
    const char id3Header[]{
        'I', 'D', '3',          // ID3 identifier
        0x03, 0x00,             // Version 2.3.0
        0x00,                   // Flags (none set)
        0x00, 0x00, 0x00, 0x00  // Size
    };

    file.write(id3Header, sizeof(id3Header));
}

void Mp3FileFormat::WriteData(std::ofstream& file, const AudioData& data) {
    if (!file.is_open()) throw std::runtime_error("MP3 file not open");
    file.write(reinterpret_cast<const char*>(data.data.data()),
        data.data.size());
}

void Mp3FileFormat::Finalize(std::ofstream& file) {
    // Not necessary for MP3 file
}


// ----- OGG FILE FORMAT ----- 
OggFileFormat::OggFileFormat() : packetNo(0), granulePos(0) {
    oggSerialNo = rand();
    ogg_stream_init(&oggStream, oggSerialNo);
}

OggFileFormat::~OggFileFormat() {
    try {
        ogg_stream_clear(&oggStream);
    }
    catch (const std::exception& e) {
        std::cout << "Error closing Ogg stream" << std::endl;
    }
}

void OggFileFormat::WriteOggPages(std::ofstream& file) {
    ogg_page oggPage;
    while (ogg_stream_pageout(&oggStream, &oggPage) > 0) {
        file.write(reinterpret_cast<const char*>(oggPage.header), oggPage.header_len);
        file.write(reinterpret_cast<const char*>(oggPage.body), oggPage.body_len);
    }
}

void OggFileFormat::FlushOggPages(std::ofstream& file) {
    ogg_page oggPage;
    while (ogg_stream_flush(&oggStream, &oggPage) > 0) {
        file.write(reinterpret_cast<const char*>(oggPage.header), oggPage.header_len);
        file.write(reinterpret_cast<const char*>(oggPage.body), oggPage.body_len);
    }
}

void OggFileFormat::WriteHeader(std::ofstream& file) {
    // OpusHead Packet 
    unsigned char header[19] = {
        'O', 'p', 'u', 's', 'H', 'e', 'a', 'd',   // Magic Signature
        1,                                        // Version
        2,                                        // Channel Count
        0x38, 0x01,                               // Pre-skip (312 samples in little-endian)
        0x80, 0xBB, 0x00, 0x00,                   // Original sample rate (48000 Hz in little-endian)
        0x00, 0x00,                               // Output gain (0)
        0                                         // Channel Mapping Family (0 = stereo)
    };

    ogg_packet oggHeader;
    oggHeader.packet = header;
    oggHeader.bytes = sizeof(header);
    oggHeader.b_o_s = 1;    // This is the beginning of the stream
    oggHeader.e_o_s = 0;
    oggHeader.granulepos = granulePos;
    oggHeader.packetno = packetNo++;

    if (ogg_stream_packetin(&oggStream, &oggHeader) != 0) {
        throw std::runtime_error("Failed to insert OpusHead into Ogg stream");
    }

    // OpusTags Packet
    const char* vendor = "MyEncoder";
    size_t vendorLen = strlen(vendor);

    size_t tagsSize = 8 + 4 + vendorLen + 4;
    std::vector<uint8_t> tagsHeader(tagsSize);

    size_t pos = 0;
    memcpy(tagsHeader.data() + pos, "Opustags", 8);
    pos += 8;

    tagsHeader[pos++] = vendorLen & 0xFF;
    tagsHeader[pos++] = (vendorLen >> 8) & 0xFF;
    tagsHeader[pos++] = (vendorLen >> 16) & 0xFF;
    tagsHeader[pos++] = (vendorLen >> 24) & 0xFF;

    memcpy(tagsHeader.data() + pos, vendor, vendorLen); 
    pos += vendorLen;

    tagsHeader[pos++] = 0;
    tagsHeader[pos++] = 0;
    tagsHeader[pos++] = 0;
    tagsHeader[pos++] = 0;

    ogg_packet oggTags;
    oggTags.packet = tagsHeader.data();
    oggTags.bytes = tagsHeader.size();
    oggTags.b_o_s = 0;
    oggTags.e_o_s = 0;
    oggTags.granulepos = granulePos;
    oggTags.packetno = packetNo++;

    if (ogg_stream_packetin(&oggStream, &oggTags) != 0) {
        throw std::runtime_error("Failed to insert OpusTags into Ogg stream");
    }

    // Flush header pages immediately and write to file
    FlushOggPages(file);

    granulePos = -OPUS_PRESKIP;
}

void OggFileFormat::WriteData(std::ofstream& file, const AudioData& data) {
    if (!file.is_open()) throw std::runtime_error(" file not open");
    if (data.frameCount == 0 || data.data.empty()) return;

    std::vector<unsigned char> packetData(data.data.begin(), data.data.end());

    ogg_packet oggData;
    oggData.packet = packetData.data();
    oggData.bytes = packetData.size();
    oggData.b_o_s = 0;
    oggData.e_o_s = 0;
    granulePos += data.frameCount;
    oggData.granulepos = granulePos;
    oggData.packetno = packetNo++;

    if (ogg_stream_packetin(&oggStream, &oggData) != 0) {
        throw std::runtime_error("Failed to insert OpusTags into Ogg stream");
    }

    ogg_page oggPage;
    WriteOggPages(file);
}

void OggFileFormat::Finalize(std::ofstream& file) {
    ogg_page oggPage;
    ogg_stream_flush(&oggStream, &oggPage);
    int result = ogg_stream_eos(&oggStream);
    FlushOggPages(file);
    ogg_stream_clear(&oggStream);
}


FileAudioSink::FileAudioSink(const std::string& filePath, std::unique_ptr<IAudioFileFormat> format)
    : filePath(filePath), format(std::move(format)) {
}

void FileAudioSink::Start() {
    outputFile.open(filePath, std::ios::binary);
    if (!outputFile) {
        throw std::runtime_error("Failed to open MP3 output file");
    }
    format->WriteHeader(outputFile);
}

void FileAudioSink::Stop() {
    format->Finalize(outputFile);
    if (outputFile.is_open()) {
        outputFile.close();
    }
}

void FileAudioSink::ConsumeAudioData(const AudioData& data) {
    format->WriteData(outputFile, data);
}

FileAudioSink::~FileAudioSink() {
    try {
        if (outputFile.is_open()) {
            outputFile.close();
        }
    }
    catch (const std::exception& e) {
        std::cout << "Error closing audio file sink" << std::endl;
    }
}
