#pragma once

#include <string>
#include <cstdint>

struct ReflectorConfig {
    std::string address;
    uint16_t port;
    uint32_t radio_id;
    std::string password;
    std::string callsign;
    int keepalive_interval;
};

struct ModemConfig {
    std::string port;
    int baud;
    uint32_t rx_frequency;
    uint32_t tx_frequency;
    int tx_power;
    int rx_offset;
    int tx_offset;
    int rf_level;
    int rx_dc_offset;
    int tx_dc_offset;
    bool enabled;
};

struct P25Config {
    uint16_t nac;
    bool enabled;
    bool trunking;
};

struct LoggingConfig {
    std::string level;
    std::string file;
    bool console;
    int max_size_mb;
    int max_files;
};

class Config {
public:
    Config();
    ~Config() = default;

    bool load(const std::string& filename);

    const ReflectorConfig& getReflector() const { return m_reflector; }
    const ModemConfig& getModem() const { return m_modem; }
    const P25Config& getP25() const { return m_p25; }
    const LoggingConfig& getLogging() const { return m_logging; }

private:
    ReflectorConfig m_reflector;
    ModemConfig m_modem;
    P25Config m_p25;
    LoggingConfig m_logging;
};
