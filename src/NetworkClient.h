#pragma once

#include "Config.h"
#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>

class NetworkClient {
public:
    using DataCallback = std::function<void(const std::vector<uint8_t>&)>;

    NetworkClient(const ReflectorConfig& config);
    ~NetworkClient();

    bool start();
    void stop();

    bool sendData(const std::vector<uint8_t>& data);
    bool isConnected() const { return m_connected.load(); }
    bool isAuthenticated() const { return m_authenticated.load(); }

    void setDataCallback(DataCallback callback) { m_dataCallback = callback; }

private:
    void receiveThread();
    void keepaliveThread();

    bool authenticate();
    void sendKeepalive();

    const ReflectorConfig& m_config;
    int m_socket;
    std::atomic<bool> m_running;
    std::atomic<bool> m_connected;
    std::atomic<bool> m_authenticated;

    std::thread m_receiveThread;
    std::thread m_keepaliveThread;

    DataCallback m_dataCallback;
    std::mutex m_sendMutex;
};
