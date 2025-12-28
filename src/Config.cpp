#include "Config.h"
#include "Logger.h"
#include <yaml-cpp/yaml.h>
#include <fstream>

Config::Config() {
    // Set defaults
    m_reflector.port = 41000;
    m_reflector.keepalive_interval = 5;

    m_modem.baud = 115200;
    m_modem.tx_power = 50;
    m_modem.rx_offset = 0;
    m_modem.tx_offset = 0;
    m_modem.rf_level = 100;
    m_modem.rx_dc_offset = 0;
    m_modem.tx_dc_offset = 0;

    m_p25.nac = 0x293;
    m_p25.enabled = true;
    m_p25.trunking = true;

    m_logging.level = "INFO";
    m_logging.console = true;
    m_logging.max_size_mb = 10;
    m_logging.max_files = 5;
}

bool Config::load(const std::string& filename) {
    try {
        YAML::Node config = YAML::LoadFile(filename);

        // Reflector settings
        if (config["reflector"]) {
            auto ref = config["reflector"];
            if (ref["address"]) m_reflector.address = ref["address"].as<std::string>();
            if (ref["port"]) m_reflector.port = ref["port"].as<uint16_t>();
            if (ref["radio_id"]) m_reflector.radio_id = ref["radio_id"].as<uint32_t>();
            if (ref["password"]) m_reflector.password = ref["password"].as<std::string>();
            if (ref["callsign"]) m_reflector.callsign = ref["callsign"].as<std::string>();
            if (ref["keepalive_interval"]) m_reflector.keepalive_interval = ref["keepalive_interval"].as<int>();
        }

        // Modem settings
        if (config["modem"]) {
            auto modem = config["modem"];
            if (modem["port"]) m_modem.port = modem["port"].as<std::string>();
            if (modem["baud"]) m_modem.baud = modem["baud"].as<int>();
            if (modem["rx_frequency"]) m_modem.rx_frequency = modem["rx_frequency"].as<uint32_t>();
            if (modem["tx_frequency"]) m_modem.tx_frequency = modem["tx_frequency"].as<uint32_t>();
            if (modem["tx_power"]) m_modem.tx_power = modem["tx_power"].as<int>();
            if (modem["rx_offset"]) m_modem.rx_offset = modem["rx_offset"].as<int>();
            if (modem["tx_offset"]) m_modem.tx_offset = modem["tx_offset"].as<int>();
            if (modem["rf_level"]) m_modem.rf_level = modem["rf_level"].as<int>();
            if (modem["rx_dc_offset"]) m_modem.rx_dc_offset = modem["rx_dc_offset"].as<int>();
            if (modem["tx_dc_offset"]) m_modem.tx_dc_offset = modem["tx_dc_offset"].as<int>();
        }

        // P25 settings
        if (config["p25"]) {
            auto p25 = config["p25"];
            if (p25["nac"]) m_p25.nac = p25["nac"].as<uint16_t>();
            if (p25["enabled"]) m_p25.enabled = p25["enabled"].as<bool>();
            if (p25["trunking"]) m_p25.trunking = p25["trunking"].as<bool>();
        }

        // Logging settings
        if (config["logging"]) {
            auto log = config["logging"];
            if (log["level"]) m_logging.level = log["level"].as<std::string>();
            if (log["file"]) m_logging.file = log["file"].as<std::string>();
            if (log["console"]) m_logging.console = log["console"].as<bool>();
            if (log["max_size_mb"]) m_logging.max_size_mb = log["max_size_mb"].as<int>();
            if (log["max_files"]) m_logging.max_files = log["max_files"].as<int>();
        }

        LOG_INFO("Configuration loaded from " + filename);
        return true;

    } catch (const YAML::Exception& e) {
        LOG_ERROR("Failed to load config: " + std::string(e.what()));
        return false;
    }
}
