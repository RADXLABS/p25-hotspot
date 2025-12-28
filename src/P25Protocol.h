#pragma once

#include <cstdint>
#include <vector>
#include <string>

// G4KLX P25 Protocol Frame Types (matching reflector)
const uint8_t FRAME_POLL = 0xF0;
const uint8_t FRAME_UNLINK = 0xF1;
const uint8_t FRAME_AUTH_REQUEST = 0xF2;
const uint8_t FRAME_AUTH_RESPONSE = 0xF3;
const uint8_t FRAME_TG_GRANT = 0xF4;
const uint8_t FRAME_TG_RELEASE = 0xF5;

// Voice/Data frames (LDU1)
const uint8_t FRAME_LDU1_0 = 0x62;
const uint8_t FRAME_LDU1_1 = 0x63;
const uint8_t FRAME_LDU1_2 = 0x64;  // LCF
const uint8_t FRAME_LDU1_3 = 0x65;  // Destination TG
const uint8_t FRAME_LDU1_4 = 0x66;  // Source ID
const uint8_t FRAME_LDU1_5 = 0x67;
const uint8_t FRAME_LDU1_6 = 0x68;
const uint8_t FRAME_LDU1_7 = 0x69;
const uint8_t FRAME_LDU1_8 = 0x6A;

// Voice/Data frames (LDU2)
const uint8_t FRAME_LDU2_0 = 0x6B;
const uint8_t FRAME_LDU2_1 = 0x6C;
const uint8_t FRAME_LDU2_2 = 0x6D;
const uint8_t FRAME_LDU2_3 = 0x6E;
const uint8_t FRAME_LDU2_4 = 0x6F;
const uint8_t FRAME_LDU2_5 = 0x70;
const uint8_t FRAME_LDU2_6 = 0x71;
const uint8_t FRAME_LDU2_7 = 0x72;
const uint8_t FRAME_LDU2_8 = 0x73;

// Trunking control frames
const uint8_t FRAME_TSBK = 0x61;

// End of transmission
const uint8_t FRAME_EOT = 0x80;

// Voice frame range
const uint8_t VOICE_FRAME_MIN = 0x62;
const uint8_t VOICE_FRAME_MAX = 0x80;

class P25Protocol {
public:
    // Build authentication request packet
    static std::vector<uint8_t> buildAuthRequest(uint32_t radioId, const std::string& password);

    // Parse authentication response
    static bool parseAuthResponse(const std::vector<uint8_t>& data, bool& authenticated);

    // Build poll/keepalive packet
    static std::vector<uint8_t> buildPollPacket();

    // Build unlink packet
    static std::vector<uint8_t> buildUnlinkPacket();

    // Check if frame is voice data
    static bool isVoiceFrame(uint8_t frameType);

    // Extract talkgroup ID from voice frame
    static uint32_t extractTalkgroupId(const std::vector<uint8_t>& data);

    // Extract source ID from voice frame
    static uint32_t extractSourceId(const std::vector<uint8_t>& data);

    // Get frame type from packet
    static uint8_t getFrameType(const std::vector<uint8_t>& data);
};
