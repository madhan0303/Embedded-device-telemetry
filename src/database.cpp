#include "database.hpp"
#include <iostream>

Database::Database(const std::string& path) {
    if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
        std::cerr << "SQLite open failed: " << sqlite3_errmsg(db_) << '\n';
        db_ = nullptr;
    }
}

Database::~Database() { if (db_) sqlite3_close(db_); }

bool Database::initialize() {
    if (!db_) return false;
    const char* sql = R"SQL(
        CREATE TABLE IF NOT EXISTS telemetry (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            device_id TEXT NOT NULL,
            sensor TEXT NOT NULL,
            value REAL NOT NULL,
            timestamp INTEGER NOT NULL
        );
        CREATE INDEX IF NOT EXISTS idx_telemetry_device_time
        ON telemetry(device_id, timestamp DESC);
    )SQL";
    char* err = nullptr;
    const int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        std::cerr << "SQLite init failed: " << (err ? err : "unknown") << '\n';
        sqlite3_free(err);
        return false;
    }
    return true;
}

bool Database::insert_telemetry(const TelemetryRecord& r) {
    if (!db_) return false;
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT INTO telemetry(device_id,sensor,value,timestamp) VALUES(?,?,?,?);";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, r.device_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, r.sensor.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, r.value);
    sqlite3_bind_int64(stmt, 4, r.timestamp);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

std::vector<TelemetryRecord> Database::latest(const std::string& device_id, int limit) {
    std::vector<TelemetryRecord> result;
    if (!db_) return result;
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT device_id,sensor,value,timestamp FROM telemetry WHERE device_id=? ORDER BY timestamp DESC,id DESC LIMIT ?;";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return result;
    sqlite3_bind_text(stmt, 1, device_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, limit);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        result.push_back({
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)),
            sqlite3_column_double(stmt, 2),
            sqlite3_column_int64(stmt, 3)
        });
    }
    sqlite3_finalize(stmt);
    return result;
}
