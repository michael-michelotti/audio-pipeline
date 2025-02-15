#include "file_format.h"
#include <iostream>
#include <iomanip>

#define NOMINMAX


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

void Mp3FileFormat::WriteData(std::ofstream& file, const MediaData& data) {
    if (!file.is_open()) throw std::runtime_error("MP3 file not open");
    file.write(reinterpret_cast<const char*>(data.data.data()),
        data.data.size());
}

void Mp3FileFormat::Finalize(std::ofstream& file) {
    // Not necessary for MP3 file
}


// ----- OGG FILE FORMAT ----- 
OggFileFormat::OggFileFormat() 
    : audioPacketNo(0)
    , audioGranulePos(0)
    , videoPacketNo(0)
    , videoGranulePos(0) {
    audioSerialNo = rand();
    do {
        videoSerialNo = rand();
    } while (videoSerialNo == audioSerialNo);

    ogg_stream_init(&audioStream, audioSerialNo);
    ogg_stream_init(&videoStream, videoSerialNo);
}

OggFileFormat::~OggFileFormat() {
    try {
        ogg_stream_clear(&audioStream);
        ogg_stream_clear(&videoStream);
    }
    catch (const std::exception& e) {
        std::cout << "Error closing Ogg stream" << std::endl;
    }
}

void OggFileFormat::WriteOggPages(std::ofstream& file, ogg_stream_state& stream) {
    ogg_page oggPage;
    int pageCount = 0;
    while (ogg_stream_pageout(&stream, &oggPage) > 0) {
        //std::cout << "Writing page " << pageCount << ":" << std::endl;
        //std::cout << "  Header size: " << oggPage.header_len << std::endl;
        //std::cout << "  Body size: " << oggPage.body_len << std::endl;
        //std::cout << "  Header: ";
        //for (int i = 0; i < oggPage.header_len; i++) {
        //    printf("%02x ", oggPage.header[i]);
        //}
        //std::cout << std::endl;
        //std::cout << "  Body: ";
        //for (int i = 0; i < oggPage.body_len; i++) {
        //    printf("%02x ", oggPage.body[i]);
        //}
        //std::cout << std::endl;

        file.write(reinterpret_cast<const char*>(oggPage.header),
            oggPage.header_len);
        file.write(reinterpret_cast<const char*>(oggPage.body),
            oggPage.body_len);
    }
}

void OggFileFormat::FlushOggPages(std::ofstream& file, ogg_stream_state& stream) {
    ogg_page oggPage;
    int pageCount = 0;
    while (ogg_stream_flush(&stream, &oggPage) > 0) {
        file.write(reinterpret_cast<const char*>(oggPage.header),
            oggPage.header_len);
        file.write(reinterpret_cast<const char*>(oggPage.body),
            oggPage.body_len);
        file.flush();
    }
}

void OggFileFormat::WriteVideoData(std::ofstream& file, const MediaData& data) {
    if (!file.is_open()) throw std::runtime_error("file not open");
    const VideoFormat& format = data.getVideoFormat();

    ogg_packet oggData;
    oggData.packet = const_cast<unsigned char*>(data.data.data());
    oggData.bytes = data.data.size();
    oggData.b_o_s = 0;
    oggData.e_o_s = 0;
    oggData.granulepos = videoGranulePos;
    // For Theora, granulepos represents frame number
    //if (format.isKeyFrame) {
    //    lastKeyFrame = videoGranulePos;
    //    oggData.granulepos = (videoGranulePos << 32) | 0;
    //}
    //else {
    //    ogg_int64_t delta = videoGranulePos - lastKeyFrame;
    //    oggData.granulepos = (lastKeyFrame << 32) | delta;
    //}
    videoGranulePos++;
    oggData.packetno = videoPacketNo++;

    //std::cout << "Writing video packet:" << std::endl;
    //std::cout << "  Size: " << data.data.size() << std::endl;
    //std::cout << "  Is keyframe: " << format.isKeyFrame << std::endl;
    //std::cout << "  Granule Pos: 0x" << std::hex << std::setfill('0') << std::setw(16)
    //    << oggData.granulepos << std::dec << std::endl;

    if (headersSet) {
        if (ogg_stream_packetin(&videoStream, &oggData) != 0) {
            throw std::runtime_error("Failed to insert video data into Ogg stream");
        }
        WriteOggPages(file, videoStream);
    }
}

void OggFileFormat::WriteAudioData(std::ofstream& file, const MediaData& data) {
    if (!file.is_open()) throw std::runtime_error("file not open");

    uint32_t frameCount;
    memcpy(&frameCount, data.data.data(), sizeof(frameCount));
    if (frameCount == 0) return;

    const uint8_t* opusData = data.data.data() + sizeof(frameCount);
    size_t opusDataSize = data.data.size() - sizeof(frameCount);

    ogg_packet oggData;
    oggData.packet = const_cast<unsigned char*>(opusData);
    oggData.bytes = opusDataSize;
    oggData.b_o_s = 0;
    oggData.e_o_s = 0;
    audioGranulePos += frameCount;
    oggData.granulepos = audioGranulePos;
    oggData.packetno = audioPacketNo++;

    if (headersSet) {
        if (ogg_stream_packetin(&audioStream, &oggData) != 0) {
            throw std::runtime_error("Failed to insert audio data into Ogg stream");
        }
        WriteOggPages(file, audioStream);
    }
}

void OggFileFormat::WriteData(std::ofstream& file, const MediaData& data) {
    if (!file.is_open()) throw std::runtime_error(" file not open");

    if (data.type == MediaData::Type::Audio) {
        WriteAudioData(file, data);
    } 
    else if (data.type == MediaData::Type::Video) {
        WriteVideoData(file, data);
    }
}

void OggFileFormat::Finalize(std::ofstream& file) {
    FlushOggPages(file, audioStream);
    FlushOggPages(file, videoStream);
    ogg_stream_clear(&audioStream);
    ogg_stream_clear(&videoStream);
}

void OggFileFormat::WriteHeader(std::ofstream& file) {
    // Opus headers are written along with Theora Headers for correct ordering
    // All written in WriteTheoraHeaders
}

void OggFileFormat::WriteOpusHead(std::ofstream& file) {
    // OpusHead Packet 
    std::cout << "write opus head" << std::endl;
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
    oggHeader.granulepos = audioGranulePos;
    oggHeader.packetno = audioPacketNo++;

    if (ogg_stream_packetin(&audioStream, &oggHeader) != 0) {
        throw std::runtime_error("Failed to insert OpusHead into Ogg stream");
    }
    FlushOggPages(file, audioStream);
}

void OggFileFormat::WriteOpusTags(std::ofstream& file) {
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
    oggTags.granulepos = audioGranulePos;
    oggTags.packetno = audioPacketNo++;

    if (ogg_stream_packetin(&audioStream, &oggTags) != 0) {
        throw std::runtime_error("Failed to insert OpusTags into Ogg stream");
    }

    FlushOggPages(file, audioStream);
    audioGranulePos = -OPUS_PRESKIP;
}

void OggFileFormat::WriteTheoraHeaders(std::ofstream& file, const MediaData& data) {
    const uint8_t* currentPos = data.data.data();
    size_t remainingSize = data.data.size();
    WriteOpusHead(file);

    for (int i = 0; i < 3; i++) {
        // Read packet size (assuming you stored it)
        if (remainingSize < sizeof(size_t)) {
            throw std::runtime_error("Malformed Theora headers data");
        }
        size_t packetSize;
        std::memcpy(&packetSize, currentPos, sizeof(size_t));
        currentPos += sizeof(size_t);
        remainingSize -= sizeof(size_t);

        if (remainingSize < packetSize) {
            throw std::runtime_error("Malformed Theora headers data");
        }

        // Create and fill the ogg packet
        ogg_packet packet;
        packet.packet = const_cast<unsigned char*>(currentPos);
        packet.bytes = packetSize;
        packet.b_o_s = (i == 0) ? 1 : 0;  // First packet is beginning of stream
        packet.e_o_s = 0;
        packet.granulepos = 0;
        packet.packetno = videoPacketNo++;

        // Write it to the stream
        if (ogg_stream_packetin(&videoStream, &packet) != 0) {
            throw std::runtime_error("Failed to write Theora header");
        }

        // Force a page break after each header
        std::cout << "flushing theora page" << std::endl;
        FlushOggPages(file, videoStream);

        if (i == 0) {
        }

        currentPos += packetSize;
        remainingSize -= packetSize;
    }

    WriteOpusTags(file);

    // Flush the pages to file
    headersSet = true;
}
