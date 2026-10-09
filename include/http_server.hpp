#pragma once
#include "database.hpp"
#include "device.hpp"
#include <atomic>
#include <string>
#include <thread>

class HttpServer {
public:
    HttpServer(int port, DeviceSimulator& device, Database& db);
    ~HttpServer();
    bool start();
    void stop();
private:
    void accept_loop();
    void handle_client(int client_fd);
    std::string route(const std::string& method, const std::string& path, const std::string& body);
    int port_;
    int server_fd_{-1};
    DeviceSimulator& device_;
    Database& db_;
    std::atomic<bool> running_{false};
    std::thread accept_thread_;
};
