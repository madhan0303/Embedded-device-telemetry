#include "device.hpp"
#include <algorithm>
#include <chrono>

DeviceSimulator::DeviceSimulator() : rng_(std::random_device{}()) {}
DeviceSimulator::~DeviceSimulator() { stop(); }

void DeviceSimulator::start() {
    if (running_.exchange(true)) return;
    worker_ = std::thread(&DeviceSimulator::sensor_loop, this);
}

void DeviceSimulator::stop() {
    if (!running_.exchange(false)) return;
    if (worker_.joinable()) worker_.join();
}

DeviceState DeviceSimulator::get_state() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return state_;
}

void DeviceSimulator::set_fan_speed(double rpm) {
    std::lock_guard<std::mutex> lock(mutex_);
    state_.fan_speed = std::clamp(rpm, 0.0, 5000.0);
}

bool DeviceSimulator::restart() {
    std::lock_guard<std::mutex> lock(mutex_);
    state_.running = true;
    state_.battery = 100.0;
    return true;
}

void DeviceSimulator::sensor_loop() {
    std::normal_distribution<double> noise(0.0, 0.8);
    std::uniform_real_distribution<double> cpu_dist(10.0, 85.0);
    while (running_) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            state_.cpu = std::clamp(cpu_dist(rng_), 0.0, 100.0);
            state_.temperature += noise(rng_) * 0.15 + (state_.cpu - 50.0) * 0.002;
            state_.temperature = std::clamp(state_.temperature, 20.0, 95.0);
            state_.battery = std::max(0.0, state_.battery - 0.02);
            if (state_.temperature > 70.0) state_.fan_speed = std::min(5000.0, state_.fan_speed + 150.0);
            else if (state_.temperature < 45.0) state_.fan_speed = std::max(800.0, state_.fan_speed - 50.0);
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
