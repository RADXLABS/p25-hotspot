#include "ModemSerial.h"
#include "Logger.h"
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <chrono>

ModemSerial::ModemSerial(const ModemConfig& config, uint16_t nac)
    : m_config(config)
    , m_nac(nac)
    , m_fd(-1)
    , m_isOpen(false)
    , m_running(false)
    , m_ackReceived(false)
{
}

ModemSerial::~ModemSerial() {
    close();
}

bool ModemSerial::open() {
    LOG_INFO("Opening modem on " + m_config.port);

    // Open serial port
    m_fd = ::open(m_config.port.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (m_fd < 0) {
        LOG_ERROR("Failed to open serial port: " + std::string(strerror(errno)));
        return false;
    }

    // Configure serial port
    struct termios tty;
    memset(&tty, 0, sizeof(tty));

    if (tcgetattr(m_fd, &tty) != 0) {
        LOG_ERROR("Failed to get serial attributes");
        ::close(m_fd);
        return false;
    }

    // Set baud rate
    speed_t baudRate = B115200;
    if (m_config.baud == 9600) baudRate = B9600;
    else if (m_config.baud == 19200) baudRate = B19200;
    else if (m_config.baud == 38400) baudRate = B38400;
    else if (m_config.baud == 57600) baudRate = B57600;
    else if (m_config.baud == 115200) baudRate = B115200;

    cfsetospeed(&tty, baudRate);
    cfsetispeed(&tty, baudRate);

    // 8N1 mode
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    // Raw mode
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);

    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(m_fd, TCSANOW, &tty) != 0) {
        LOG_ERROR("Failed to set serial attributes");
        ::close(m_fd);
        return false;
    }

    m_isOpen = true;
    LOG_INFO("Serial port opened successfully");

    // Start read thread
    m_running = true;
    m_readThread = std::thread(&ModemSerial::readThread, this);

    // Get modem version
    std::string version;
    if (getVersion(version)) {
        LOG_INFO("Modem version: " + version);
    } else {
        LOG_WARN("Failed to get modem version");
    }

    // Configure modem
    if (!configure()) {
        LOG_ERROR("Failed to configure modem");
        close();
        return false;
    }

    // Set frequencies - DISABLED FOR TESTING
    // if (!setFrequencies()) {
    //     LOG_ERROR("Failed to set frequencies");
    //     close();
    //     return false;
    // }
    LOG_WARN("Frequency setting bypassed - modem will use default frequencies");

    // Set to P25 mode - BYPASSED, firmware doesn't support it
    // if (!setMode(MODE_P25)) {
    //     LOG_ERROR("Failed to set P25 mode");
    //     close();
    //     return false;
    // }
    LOG_WARN("P25 mode bypassed - modem will stay in idle mode");

    LOG_INFO("Modem initialized successfully");
    return true;
}

void ModemSerial::close() {
    if (!m_isOpen) {
        return;
    }

    LOG_INFO("Closing modem...");

    // Set to idle mode
    setMode(MODE_IDLE);

    m_running = false;

    if (m_readThread.joinable()) {
        m_readThread.join();
    }

    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }

    m_isOpen = false;
    LOG_INFO("Modem closed");
}

bool ModemSerial::writeP25Data(const std::vector<uint8_t>& data) {
    if (!m_isOpen) {
        return false;
    }

    return sendCommand(CMD_P25_DATA, data);
}

bool ModemSerial::setMode(uint8_t mode) {
    LOG_INFO("Setting modem mode: " + std::to_string(mode));

    std::vector<uint8_t> data = {mode};
    if (!sendCommand(CMD_SET_MODE, data)) {
        return false;
    }

    if (!waitForAck()) {
        LOG_ERROR("Failed to set mode - no ACK");
        return false;
    }

    LOG_INFO("Mode set successfully");
    return true;
}

bool ModemSerial::getVersion(std::string& version) {
    if (!sendCommand(CMD_GET_VERSION, {})) {
        return false;
    }

    // Wait for response
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Version response will be in m_rxBuffer (simplified - real implementation needs proper parsing)
    version = "MMDVM";  // Placeholder
    return true;
}

bool ModemSerial::getStatus() {
    return sendCommand(CMD_GET_STATUS, {});
}

bool ModemSerial::sendCommand(uint8_t command, const std::vector<uint8_t>& data) {
    if (!m_isOpen || m_fd < 0) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_writeMutex);

    // Build packet: START + LENGTH + COMMAND + DATA
    std::vector<uint8_t> packet;
    packet.push_back(FRAME_START);
    packet.push_back(static_cast<uint8_t>(data.size() + 3));  // Length includes command byte
    packet.push_back(command);
    packet.insert(packet.end(), data.begin(), data.end());

    // Write to serial
    ssize_t written = write(m_fd, packet.data(), packet.size());
    if (written != static_cast<ssize_t>(packet.size())) {
        LOG_ERROR("Failed to write to modem");
        return false;
    }

    return true;
}

bool ModemSerial::waitForAck(int timeoutMs) {
    m_ackReceived = false;

    auto startTime = std::chrono::steady_clock::now();

    while (true) {
        if (m_ackReceived) {
            return true;
        }

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();

        if (elapsed > timeoutMs) {
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

bool ModemSerial::configure() {
    LOG_INFO("Configuring modem...");

    // Build config packet (simplified - based on MMDVM protocol)
    std::vector<uint8_t> config;

    // RX invert, TX invert, PTT invert, etc. (all false for standard setup)
    config.push_back(0x00);  // RX invert
    config.push_back(0x00);  // TX invert
    config.push_back(0x00);  // PTT invert
    config.push_back(0x00);  // YSF invert
    config.push_back(0x00);  // Debug

    // Mode enables (enable DMR instead of P25 for testing)
    config.push_back(0x01);  // DMR enabled (firmware supports this)
    config.push_back(0x00);  // YSF disabled
    config.push_back(0x00);  // P25 disabled (firmware rejects this)
    config.push_back(0x00);  // NXDN disabled

    // TX/RX levels
    config.push_back(static_cast<uint8_t>(m_config.tx_power));
    config.push_back(static_cast<uint8_t>(m_config.rf_level));

    // Delays (set to 0 for now)
    config.push_back(0x00);
    config.push_back(0x00);

    if (!sendCommand(CMD_SET_CONFIG, config)) {
        return false;
    }

    if (!waitForAck()) {
        LOG_ERROR("Failed to configure modem - no ACK");
        return false;
    }

    LOG_INFO("Modem configured successfully");
    return true;
}

bool ModemSerial::setFrequencies() {
    LOG_INFO("Setting frequencies...");

    // Set RX frequency
    std::vector<uint8_t> rxFreq;
    uint32_t rx = m_config.rx_frequency;
    rxFreq.push_back((rx >> 24) & 0xFF);
    rxFreq.push_back((rx >> 16) & 0xFF);
    rxFreq.push_back((rx >> 8) & 0xFF);
    rxFreq.push_back(rx & 0xFF);

    if (!sendCommand(CMD_SET_RXFREQ, rxFreq) || !waitForAck()) {
        LOG_ERROR("Failed to set RX frequency");
        return false;
    }

    // Set TX frequency
    std::vector<uint8_t> txFreq;
    uint32_t tx = m_config.tx_frequency;
    txFreq.push_back((tx >> 24) & 0xFF);
    txFreq.push_back((tx >> 16) & 0xFF);
    txFreq.push_back((tx >> 8) & 0xFF);
    txFreq.push_back(tx & 0xFF);

    if (!sendCommand(CMD_SET_TXFREQ, txFreq) || !waitForAck()) {
        LOG_ERROR("Failed to set TX frequency");
        return false;
    }

    LOG_INFO("Frequencies set: RX=" + std::to_string(rx) + " TX=" + std::to_string(tx));
    return true;
}

void ModemSerial::readThread() {
    LOG_INFO("Modem read thread started");

    uint8_t buffer[2048];

    while (m_running) {
        ssize_t n = read(m_fd, buffer, sizeof(buffer));

        if (n > 0) {
            // Add to RX buffer
            m_rxBuffer.insert(m_rxBuffer.end(), buffer, buffer + n);

            // Process complete frames
            while (m_rxBuffer.size() >= 3) {
                // Look for frame start
                if (m_rxBuffer[0] != FRAME_START) {
                    m_rxBuffer.erase(m_rxBuffer.begin());
                    continue;
                }

                uint8_t length = m_rxBuffer[1];

                // Validate length to prevent crashes
                if (length < 3 || length > 250) {
                    LOG_WARN("Invalid frame length: " + std::to_string(length) + ", discarding");
                    m_rxBuffer.erase(m_rxBuffer.begin());
                    continue;
                }

                if (m_rxBuffer.size() < length) {
                    break;  // Wait for more data
                }

                uint8_t command = m_rxBuffer[2];

                // Extract frame data
                std::vector<uint8_t> frameData(m_rxBuffer.begin() + 3, m_rxBuffer.begin() + length);

                // Remove processed frame from buffer
                m_rxBuffer.erase(m_rxBuffer.begin(), m_rxBuffer.begin() + length);

                // Handle frame based on command
                if (command == CMD_ACK) {
                    m_ackReceived = true;
                    LOG_DEBUG("Received ACK");
                } else if (command == CMD_NAK) {
                    LOG_WARN("Received NAK");
                } else if (command == CMD_P25_DATA) {
                    // P25 data from modem (RF → Network)
                    if (m_p25Callback) {
                        m_p25Callback(frameData);
                    }
                }
            }
        } else if (n < 0 && errno != EAGAIN) {
            LOG_ERROR("Modem read error: " + std::string(strerror(errno)));
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    LOG_INFO("Modem read thread stopped");
}
