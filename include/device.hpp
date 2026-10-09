#pragma once
#include <atomic>
#include <mutex>
#include <random>
#include <string>
#include <thread>

struct DeviceState {
    double temperature{25.0};
    double battery{100.0};
    double cpu{0.0};
    double fan_speed{1200.0};
    bool running{true};
};

class DeviceSimulator {
public:
    DeviceSimulator();
    ~DeviceSimulator();
    void start();
    void stop();
    DeviceState get_state() const;
    void set_fan_speed(double rpm);
    bool restart();
private:
    void sensor_loop();
    mutable std::mutex mutex_;
    DeviceState state_;
    std::atomic<bool> running_{false};
    std::thread worker_;
    std::mt19937 rng_;
};
