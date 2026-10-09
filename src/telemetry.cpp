#include "telemetry.hpp"

void TelemetryQueue::push(TelemetryRecord record) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopped_) return;
        queue_.push(std::move(record));
    }
    cv_.notify_one();
}

bool TelemetryQueue::pop(TelemetryRecord& record) {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [&] { return stopped_ || !queue_.empty(); });
    if (queue_.empty()) return false;
    record = std::move(queue_.front());
    queue_.pop();
    return true;
}

void TelemetryQueue::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopped_ = true;
    }
    cv_.notify_all();
}
