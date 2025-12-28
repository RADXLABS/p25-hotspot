#pragma once

#include "Config.h"
#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>

// MMDVM protocol commands (based on G4KLX protocol)
const uint8_t CMD_GET_VERSION = 0x00;
const uint8_t CMD_GET_STATUS = 0x01;
const uint8_t CMD_SET_CONFIG = 0x02;
const uint8_t CMD_SET_MODE = 0x03;
const uint8_t CMD_SET_RXFREQ = 0x04;
const uint8_t CMD_SET_TXFREQ = 0x05;
const uint8_t CMD_CAL_DATA = 0x08;
const uint8_t CMD_SEND_CWID = 0x0A;
const uint8_t CMD_P25_DATA = 0x31;
const uint8_t CMD_P25_LOST = 0x32;
const uint8_t CMD_ACK = 0x70;
const uint8_t CMD_NAK = 0x7F;

// Modem modes
const uint8_t MODE_IDLE = 0;
const uint8_t MODE_P25 = 4;

// Frame start/end markers
const uint8_t FRAME_START = 0xE0;

class ModemSerial {
public:
    using P25DataCallback = std::function<void(const std::vector<uint8_t>&)>;

    ModemSerial(const ModemConfig& config, uint16_t nac);
    ~ModemSerial();

    bool open();
    void close();

    bool isOpen() const { return m_isOpen.load(); }

    // Send P25 data to modem (to be transmitted over RF)
    bool writeP25Data(const std::vector<uint8_t>& data);

    // Set callback for P25 data received from modem (from RF)
    void setP25DataCallback(P25DataCallback callback) { m_p25Callback = callback; }

    // Modem control
    bool setMode(uint8_t mode);
    bool getVersion(std::string& version);
    bool getStatus();

private:
    void readThread();

    bool sendCommand(uint8_t command, const std::vector<uint8_t>& data);
    bool waitForAck(int timeoutMs = 1000);

    bool configure();
    bool setFrequencies();

    const ModemConfig& m_config;
    uint16_t m_nac;

    int m_fd;
    std::atomic<bool> m_isOpen;
    std::atomic<bool> m_running;

    std::thread m_readThread;
    std::mutex m_writeMutex;

    P25DataCallback m_p25Callback;

    // Response handling
    std::vector<uint8_t> m_rxBuffer;
    std::atomic<bool> m_ackReceived;
};
