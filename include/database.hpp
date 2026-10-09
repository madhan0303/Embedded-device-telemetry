#pragma once
#include "telemetry.hpp"
#include <string>
#include <vector>
#include <sqlite3.h>

class Database {
public:
    explicit Database(const std::string& path);
    ~Database();
    bool initialize();
    bool insert_telemetry(const TelemetryRecord& record);
    std::vector<TelemetryRecord> latest(const std::string& device_id, int limit = 20);
private:
    sqlite3* db_{nullptr};
};
