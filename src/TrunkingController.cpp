#include "TrunkingController.h"
#include "P25Protocol.h"
#include "Logger.h"

TrunkingController::TrunkingController(
    const P25Config& config,
    std::shared_ptr<ModemSerial> modem,
    std::shared_ptr<NetworkClient> network)
    : m_config(config)
    , m_modem(modem)
    , m_network(network)
    , m_running(false)
    , m_currentTalkgroup(0)
    , m_inCall(false)
{
}

void TrunkingController::start() {
    LOG_INFO("Starting trunking controller...");

    m_running = true;

    // Set up callbacks
    m_modem->setP25DataCallback([this](const std::vector<uint8_t>& data) {
        handleModemData(data);
    });

    m_network->setDataCallback([this](const std::vector<uint8_t>& data) {
        handleNetworkData(data);
    });

    LOG_INFO("Trunking controller started");
}

void TrunkingController::stop() {
    if (!m_running) {
        return;
    }

    LOG_INFO("Stopping trunking controller...");
    m_running = false;
    LOG_INFO("Trunking controller stopped");
}

void TrunkingController::handleModemData(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return;
    }

    uint8_t frameType = P25Protocol::getFrameType(data);

    // Voice frames from RF → send to network
    if (P25Protocol::isVoiceFrame(frameType)) {
        handleVoiceFrame(data);

        // Forward to network
        if (m_network->isAuthenticated()) {
            m_network->sendData(data);
        }
    }
    // TSBK frames
    else if (frameType == FRAME_TSBK) {
        processTSBK(data);
    }
    // End of transmission
    else if (frameType == FRAME_EOT) {
        if (m_inCall) {
            LOG_INFO("End of transmission on TG " + std::to_string(m_currentTalkgroup.load()));
            m_inCall = false;
            m_currentTalkgroup = 0;
        }

        // Forward EOT to network
        if (m_network->isAuthenticated()) {
            m_network->sendData(data);
        }
    }
}

void TrunkingController::handleNetworkData(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return;
    }

    uint8_t frameType = P25Protocol::getFrameType(data);

    // Ignore auth frames (already handled by NetworkClient)
    if (frameType == FRAME_AUTH_RESPONSE || frameType == FRAME_AUTH_REQUEST) {
        return;
    }

    // Ignore poll frames
    if (frameType == FRAME_POLL) {
        return;
    }

    // Voice frames from network → send to modem (RF)
    if (P25Protocol::isVoiceFrame(frameType)) {
        if (m_modem->isOpen()) {
            m_modem->writeP25Data(data);
        }
    }
    // Talkgroup grant notifications
    else if (frameType == FRAME_TG_GRANT) {
        LOG_INFO("Received talkgroup grant from network");
        processTSBK(data);
    }
    // TSBK frames
    else if (frameType == FRAME_TSBK) {
        processTSBK(data);

        // Forward TSBK to modem for RF transmission (if trunking enabled)
        if (m_config.trunking && m_modem->isOpen()) {
            m_modem->writeP25Data(data);
        }
    }
    // EOT from network
    else if (frameType == FRAME_EOT) {
        if (m_modem->isOpen()) {
            m_modem->writeP25Data(data);
        }
    }
}

void TrunkingController::processTSBK(const std::vector<uint8_t>& data) {
    if (data.size() < 12) {
        return;
    }

    // Simple TSBK processing (real implementation would decode full TSBK structure)
    // For now, just log that we received it
    LOG_DEBUG("Processing TSBK message");

    // In a real system, this would:
    // - Decode TSBK opcode
    // - Handle different TSBK message types (Group Voice Grant, etc.)
    // - Manage trunking state
    // - Switch channels as needed
}

void TrunkingController::handleVoiceFrame(const std::vector<uint8_t>& data) {
    // Extract talkgroup and source from voice frame
    uint32_t tg = P25Protocol::extractTalkgroupId(data);
    uint32_t src = P25Protocol::extractSourceId(data);

    if (tg > 0 && !m_inCall) {
        m_inCall = true;
        m_currentTalkgroup = tg;
        LOG_INFO("Voice call started - TG: " + std::to_string(tg) + " SRC: " + std::to_string(src));
    }
}
