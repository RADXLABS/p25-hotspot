#include "Config.h"
#include "Logger.h"
#include "ModemSerial.h"
#include "NetworkClient.h"
#include "TrunkingController.h"
#include <iostream>
#include <signal.h>
#include <memory>
#include <atomic>
#include <thread>
#include <chrono>

// Global flag for signal handling
std::atomic<bool> g_running(true);

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nReceived shutdown signal..." << std::endl;
        g_running = false;
    }
}

LogLevel parseLogLevel(const std::string& level) {
    if (level == "DEBUG") return LogLevel::DEBUG;
    if (level == "INFO") return LogLevel::INFO;
    if (level == "WARN") return LogLevel::WARN;
    if (level == "ERROR") return LogLevel::ERROR;
    return LogLevel::INFO;
}

int main(int argc, char* argv[]) {
    std::cout << "============================================================" << std::endl;
    std::cout << "  P25 Hotspot Software v1.0.0" << std::endl;
    std::cout << "  Built for radxrf.com P25 Trunking Network" << std::endl;
    std::cout << "============================================================" << std::endl;
    std::cout << std::endl;

    // Parse command line arguments
    std::string configFile = "/etc/p25-hotspot.yaml";
    if (argc > 1) {
        configFile = argv[1];
    }

    std::cout << "Using config file: " << configFile << std::endl;

    // Load configuration
    Config config;
    if (!config.load(configFile)) {
        std::cerr << "Failed to load configuration from " << configFile << std::endl;
        std::cerr << "Try: p25-hotspot /path/to/config.yaml" << std::endl;
        return 1;
    }

    // Initialize logger
    LogLevel logLevel = parseLogLevel(config.getLogging().level);
    Logger::getInstance().init(
        config.getLogging().file,
        logLevel,
        config.getLogging().console
    );

    LOG_INFO("============================================================");
    LOG_INFO("P25 Hotspot Starting");
    LOG_INFO("============================================================");

    // Register signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Create components
    auto modem = std::make_shared<ModemSerial>(
        config.getModem(),
        config.getP25().nac
    );

    auto network = std::make_shared<NetworkClient>(
        config.getReflector()
    );

    auto controller = std::make_shared<TrunkingController>(
        config.getP25(),
        modem,
        network
    );

    // Start modem
    LOG_INFO("Initializing MMDVM modem...");
    if (!modem->open()) {
        LOG_ERROR("Failed to open modem - exiting");
        return 1;
    }

    // Start network
    LOG_INFO("Connecting to reflector...");
    if (!network->start()) {
        LOG_ERROR("Failed to connect to reflector - exiting");
        modem->close();
        return 1;
    }

    // Wait for authentication
    int authWait = 0;
    while (!network->isAuthenticated() && authWait < 10) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        authWait++;
    }

    if (!network->isAuthenticated()) {
        LOG_ERROR("Authentication timeout - exiting");
        network->stop();
        modem->close();
        return 1;
    }

    // Start trunking controller
    LOG_INFO("Starting trunking controller...");
    controller->start();

    LOG_INFO("============================================================");
    LOG_INFO("✓ P25 Hotspot Running");
    LOG_INFO("============================================================");
    LOG_INFO("Reflector: " + config.getReflector().address + ":" + std::to_string(config.getReflector().port));
    LOG_INFO("Radio ID: " + std::to_string(config.getReflector().radio_id) + " (" + config.getReflector().callsign + ")");
    LOG_INFO("Modem: " + config.getModem().port);
    LOG_INFO("RX Freq: " + std::to_string(config.getModem().rx_frequency / 1000000.0) + " MHz");
    LOG_INFO("TX Freq: " + std::to_string(config.getModem().tx_frequency / 1000000.0) + " MHz");
    LOG_INFO("NAC: 0x" + std::to_string(config.getP25().nac));
    LOG_INFO("Trunking: " + std::string(config.getP25().trunking ? "Enabled" : "Disabled"));
    LOG_INFO("============================================================");
    LOG_INFO("Press Ctrl+C to stop");
    LOG_INFO("");

    // Main loop
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // Check modem status
        if (!modem->isOpen()) {
            LOG_ERROR("Modem connection lost - exiting");
            break;
        }

        // Check network status
        if (!network->isConnected()) {
            LOG_ERROR("Network connection lost - exiting");
            break;
        }
    }

    // Shutdown
    LOG_INFO("");
    LOG_INFO("============================================================");
    LOG_INFO("Shutting down...");
    LOG_INFO("============================================================");

    controller->stop();
    network->stop();
    modem->close();

    LOG_INFO("✓ P25 Hotspot stopped cleanly");
    LOG_INFO("73!");

    return 0;
}
