#include "NetworkClient.h"
#include "P25Protocol.h"
#include "Logger.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <chrono>

NetworkClient::NetworkClient(const ReflectorConfig& config)
    : m_config(config)
    , m_socket(-1)
    , m_running(false)
    , m_connected(false)
    , m_authenticated(false)
{
}

NetworkClient::~NetworkClient() {
    stop();
}

bool NetworkClient::start() {
    LOG_INFO("Starting network client...");

    // Create UDP socket
    m_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_socket < 0) {
        LOG_ERROR("Failed to create socket");
        return false;
    }

    // Set socket to non-blocking for receive
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;  // 100ms timeout
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // Connect to reflector
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(m_config.port);

    if (inet_pton(AF_INET, m_config.address.c_str(), &serverAddr.sin_addr) <= 0) {
        LOG_ERROR("Invalid reflector address: " + m_config.address);
        close(m_socket);
        return false;
    }

    // "Connect" the UDP socket (sets default destination)
    if (connect(m_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        LOG_ERROR("Failed to connect to reflector");
        close(m_socket);
        return false;
    }

    m_connected = true;
    LOG_INFO("Connected to reflector at " + m_config.address + ":" + std::to_string(m_config.port));

    // Authenticate
    if (!authenticate()) {
        LOG_ERROR("Authentication failed");
        stop();
        return false;
    }

    // Start threads
    m_running = true;
    m_receiveThread = std::thread(&NetworkClient::receiveThread, this);
    m_keepaliveThread = std::thread(&NetworkClient::keepaliveThread, this);

    LOG_INFO("Network client started successfully");
    return true;
}

void NetworkClient::stop() {
    if (!m_running) {
        return;
    }

    LOG_INFO("Stopping network client...");
    m_running = false;

    // Send unlink packet
    if (m_authenticated) {
        auto unlinkPacket = P25Protocol::buildUnlinkPacket();
        sendData(unlinkPacket);
    }

    // Wait for threads
    if (m_receiveThread.joinable()) {
        m_receiveThread.join();
    }
    if (m_keepaliveThread.joinable()) {
        m_keepaliveThread.join();
    }

    // Close socket
    if (m_socket >= 0) {
        close(m_socket);
        m_socket = -1;
    }

    m_connected = false;
    m_authenticated = false;

    LOG_INFO("Network client stopped");
}

bool NetworkClient::sendData(const std::vector<uint8_t>& data) {
    if (!m_connected || m_socket < 0) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_sendMutex);

    ssize_t sent = send(m_socket, data.data(), data.size(), 0);
    if (sent < 0) {
        LOG_ERROR("Failed to send data to reflector");
        return false;
    }

    return true;
}

bool NetworkClient::authenticate() {
    LOG_INFO("Authenticating with reflector...");
    LOG_INFO("Radio ID: " + std::to_string(m_config.radio_id) + " (" + m_config.callsign + ")");

    // Build and send auth request
    auto authPacket = P25Protocol::buildAuthRequest(m_config.radio_id, m_config.password);
    if (!sendData(authPacket)) {
        LOG_ERROR("Failed to send auth request");
        return false;
    }

    // Wait for response (max 5 seconds)
    auto startTime = std::chrono::steady_clock::now();
    const int timeoutMs = 5000;

    std::vector<uint8_t> buffer(1024);

    while (true) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();

        if (elapsed > timeoutMs) {
            LOG_ERROR("Authentication timeout");
            return false;
        }

        ssize_t received = recv(m_socket, buffer.data(), buffer.size(), 0);
        if (received > 0) {
            buffer.resize(received);

            // Check if this is an auth response
            if (buffer[0] == FRAME_AUTH_RESPONSE) {
                bool authenticated = false;
                if (P25Protocol::parseAuthResponse(buffer, authenticated)) {
                    if (authenticated) {
                        m_authenticated = true;
                        LOG_INFO("✓ Authentication successful!");
                        return true;
                    } else {
                        LOG_ERROR("✗ Authentication rejected by server");
                        return false;
                    }
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return false;
}

void NetworkClient::receiveThread() {
    LOG_INFO("Receive thread started");

    std::vector<uint8_t> buffer(2048);

    while (m_running) {
        ssize_t received = recv(m_socket, buffer.data(), buffer.size(), 0);

        if (received > 0) {
            buffer.resize(received);

            // Call data callback if set
            if (m_dataCallback) {
                m_dataCallback(buffer);
            }

            buffer.resize(2048);  // Reset buffer size
        } else if (received < 0) {
            // Check if it's just a timeout (expected with SO_RCVTIMEO)
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                LOG_ERROR("Receive error: " + std::string(strerror(errno)));
                break;
            }
        }
    }

    LOG_INFO("Receive thread stopped");
}

void NetworkClient::keepaliveThread() {
    LOG_INFO("Keepalive thread started");

    while (m_running) {
        if (m_authenticated) {
            sendKeepalive();
        }

        // Sleep for keepalive interval
        for (int i = 0; i < m_config.keepalive_interval && m_running; i++) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    LOG_INFO("Keepalive thread stopped");
}

void NetworkClient::sendKeepalive() {
    auto pollPacket = P25Protocol::buildPollPacket();
    if (!sendData(pollPacket)) {
        LOG_WARN("Failed to send keepalive");
    } else {
        LOG_DEBUG("Sent keepalive");
    }
}
