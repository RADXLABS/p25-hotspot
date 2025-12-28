#pragma once

#include "ModemSerial.h"
#include "NetworkClient.h"
#include "Config.h"
#include <memory>
#include <atomic>

class TrunkingController {
public:
    TrunkingController(
        const P25Config& config,
        std::shared_ptr<ModemSerial> modem,
        std::shared_ptr<NetworkClient> network
    );
    ~TrunkingController() = default;

    void start();
    void stop();

private:
    // Callbacks
    void handleModemData(const std::vector<uint8_t>& data);
    void handleNetworkData(const std::vector<uint8_t>& data);

    // Trunking logic
    void processTSBK(const std::vector<uint8_t>& data);
    void handleVoiceFrame(const std::vector<uint8_t>& data);

    const P25Config& m_config;
    std::shared_ptr<ModemSerial> m_modem;
    std::shared_ptr<NetworkClient> m_network;

    std::atomic<bool> m_running;

    // Current call state
    std::atomic<uint32_t> m_currentTalkgroup;
    std::atomic<bool> m_inCall;
};
