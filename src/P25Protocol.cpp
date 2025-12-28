#include "P25Protocol.h"
#include "Logger.h"
#include <cstring>

std::vector<uint8_t> P25Protocol::buildAuthRequest(uint32_t radioId, const std::string& password) {
    // Format: 0xF2 + 4 bytes radio_id (big-endian) + password (null-terminated)
    std::vector<uint8_t> packet;
    packet.push_back(FRAME_AUTH_REQUEST);

    // Add radio ID (big-endian)
    packet.push_back((radioId >> 24) & 0xFF);
    packet.push_back((radioId >> 16) & 0xFF);
    packet.push_back((radioId >> 8) & 0xFF);
    packet.push_back(radioId & 0xFF);

    // Add password (null-terminated)
    for (char c : password) {
        packet.push_back(static_cast<uint8_t>(c));
    }
    packet.push_back(0x00);  // Null terminator

    return packet;
}

bool P25Protocol::parseAuthResponse(const std::vector<uint8_t>& data, bool& authenticated) {
    if (data.size() < 2) {
        return false;
    }

    if (data[0] != FRAME_AUTH_RESPONSE) {
        return false;
    }

    // 0x01 = success, 0x00 = failure
    authenticated = (data[1] == 0x01);
    return true;
}

std::vector<uint8_t> P25Protocol::buildPollPacket() {
    std::vector<uint8_t> packet;
    packet.push_back(FRAME_POLL);
    return packet;
}

std::vector<uint8_t> P25Protocol::buildUnlinkPacket() {
    std::vector<uint8_t> packet;
    packet.push_back(FRAME_UNLINK);
    return packet;
}

bool P25Protocol::isVoiceFrame(uint8_t frameType) {
    return frameType >= VOICE_FRAME_MIN && frameType <= VOICE_FRAME_MAX;
}

uint32_t P25Protocol::extractTalkgroupId(const std::vector<uint8_t>& data) {
    // Talkgroup ID is typically in bytes 5-6 (16-bit) for P25
    // This is simplified - real P25 has more complex frame structure
    if (data.size() < 7) {
        return 0;
    }

    uint32_t tg = 0;
    tg = (data[5] << 8) | data[6];
    return tg;
}

uint32_t P25Protocol::extractSourceId(const std::vector<uint8_t>& data) {
    // Source ID is typically in bytes 7-9 (24-bit) for P25
    // This is simplified - real P25 has more complex frame structure
    if (data.size() < 10) {
        return 0;
    }

    uint32_t src = 0;
    src = (data[7] << 16) | (data[8] << 8) | data[9];
    return src;
}

uint8_t P25Protocol::getFrameType(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return 0;
    }
    return data[0];
}
