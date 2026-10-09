#include "database.hpp"
#include "device.hpp"
#include "http_server.hpp"
#include "telemetry.hpp"
#include <chrono>
#include <csignal>
#include <ctime>
#include <iostream>
#include <thread>

namespace { volatile std::sig_atomic_t stop_requested = 0; }
void signal_handler(int) { stop_requested = 1; }

int main(int argc, char** argv) {
    int port = 8080;
    if (argc > 1) port = std::stoi(argv[1]);

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    Database db("data/telemetry.db");
    if (!db.initialize()) return 1;

    DeviceSimulator device;
    TelemetryQueue queue;
    device.start();

    std::thread collector([&] {
        while (!stop_requested) {
            const DeviceState state = device.get_state();
            const auto ts = static_cast<std::int64_t>(std::time(nullptr));
            queue.push({"edge-01", "temperature", state.temperature, ts});
            queue.push({"edge-01", "battery", state.battery, ts});
            queue.push({"edge-01", "cpu", state.cpu, ts});
            queue.push({"edge-01", "fan_speed", state.fan_speed, ts});
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
        queue.stop();
    });

    std::thread writer([&] {
        TelemetryRecord record;
        while (queue.pop(record)) db.insert_telemetry(record);
    });

    HttpServer server(port, device, db);
    if (!server.start()) {
        std::cerr << "Failed to start HTTP server on port " << port << '\n';
        stop_requested = 1;
    } else {
        std::cout << "Edge device server listening on http://127.0.0.1:" << port << '\n';
        std::cout << "GET  /api/device/status\nGET  /api/telemetry\n"
                     "POST /api/device/restart\nPOST /api/device/fan {\"rpm\":2000}\n";
    }

    while (!stop_requested) std::this_thread::sleep_for(std::chrono::milliseconds(200));
    server.stop();
    device.stop();
    if (collector.joinable()) collector.join();
    queue.stop();
    if (writer.joinable()) writer.join();
    return 0;
}
