#pragma once
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <queue>
#include <string>

struct TelemetryRecord {
    std::string device_id;
    std::string sensor;
    double value;
    std::int64_t timestamp;
};

class TelemetryQueue {
public:
    void push(TelemetryRecord record);
    bool pop(TelemetryRecord& record);
    void stop();
private:
    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<TelemetryRecord> queue_;
    bool stopped_{false};
};
