#include <iostream>
#include <stdexcept>

#include <ogg/ogg.h>

#include "media_pipeline/file_formats/ogg_format.h"
#include "media_pipeline/core/interfaces/i_file_format.h"
#include "media_pipeline/core/media_data.h"

namespace media_pipeline::file_formats {
    using core::VideoFormat;
    using core::MediaData;

    OggFileFormat::OggFileFormat()
        : audioPacketNo(0)
        , audioGranulePos(0)
        , videoPacketNo(0)
        , videoGranulePos(0)
        , relativePackets(0)
        , totalPackets(0) {
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


            if (&stream == &audioStream) {
                static int audioPageNo = 0;
                std::cout << "wrote audio page number " << ++audioPageNo << std::endl;
            }
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
        //if (&stream == &videoStream) {
        //    std::cout << "wrote frame number " << videoPacketNo << std::endl;
        //}
    }

    void OggFileFormat::WriteVideoData(std::ofstream& file, const MediaData& data) {
        if (!file.is_open()) throw std::runtime_error("file not open");

        if (headersSet) {
            const VideoFormat& format = data.getVideoFormat();

            ogg_packet oggData;
            oggData.packet = const_cast<unsigned char*>(data.data.data());
            oggData.bytes = data.data.size();
            oggData.b_o_s = 0;
            oggData.e_o_s = 0;
            oggData.granulepos = format.granulepos;
            oggData.packetno = videoPacketNo++;

            if (ogg_stream_packetin(&videoStream, &oggData) != 0) {
                throw std::runtime_error("Failed to insert video data into Ogg stream");
            }
            FlushOggPages(file, videoStream);
        }
    }

    void OggFileFormat::WriteAudioData(std::ofstream& file, const MediaData& data) {
        if (!file.is_open()) throw std::runtime_error("file not open");

        if (headersSet) {
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

            double timestamp = (double)audioGranulePos / (double)48000;
            std::cout << "Audio granule pos for packet " << audioPacketNo << ": " << audioGranulePos << std::endl;
            std::cout << "Audio timestamp: " << timestamp << std::endl;

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
        // Mark last audio packet as end of stream
        ogg_packet lastAudioPacket;
        lastAudioPacket.e_o_s = 1;  // Set end of stream flag
        lastAudioPacket.bytes = 0;
        lastAudioPacket.b_o_s = 0;
        lastAudioPacket.granulepos = audioGranulePos;
        lastAudioPacket.packetno = audioPacketNo++;
        ogg_stream_packetin(&audioStream, &lastAudioPacket);

        // Mark last video packet as end of stream
        ogg_packet lastVideoPacket;
        lastVideoPacket.e_o_s = 1;  // Set end of stream flag
        lastVideoPacket.bytes = 0;
        lastVideoPacket.b_o_s = 0;
        lastVideoPacket.granulepos = videoGranulePos;
        lastVideoPacket.packetno = videoPacketNo++;
        ogg_stream_packetin(&videoStream, &lastVideoPacket);

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
        memcpy(tagsHeader.data() + pos, "OpusTags", 8);
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

    ogg_packet OggFileFormat::GenerateEmptyPacket() {
        // Mark last audio packet as end of stream
        ogg_packet emptyPacket;
        emptyPacket.e_o_s = 0;  // Set end of stream flag
        emptyPacket.bytes = 0;
        emptyPacket.b_o_s = 0;
        emptyPacket.granulepos = 0;
        emptyPacket.packetno = 0;
        return emptyPacket;
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
            FlushOggPages(file, videoStream);

            currentPos += packetSize;
            remainingSize -= packetSize;
        }

        ogg_packet pageBreak = GenerateEmptyPacket();
        pageBreak.packetno = videoPacketNo++;
        if (ogg_stream_packetin(&videoStream, &pageBreak) != 0) {
            throw std::runtime_error("Failed to write Theora header");
        }
        // Force a page break after each header
        FlushOggPages(file, videoStream);

        WriteOpusTags(file);

        // Flush the pages to file
        headersSet = true;
    }
}
