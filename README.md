# Embedded Device Telemetry & Control System

A Linux/C++17 edge-device simulation that collects device telemetry concurrently, persists it in SQLite, and exposes a small HTTP API for monitoring and control.

## Architecture

```text
             Device Simulator
        ┌──────────┬───────────┐
        │ sensor loop          │
        └──────────┬───────────┘
                   │
                   ▼
             Telemetry Queue
                   │
                   ▼
             Database Writer
                   │
                   ▼
                SQLite

       HTTP Client ──> HTTP Server
                         │
                 ┌───────┴────────┐
                 ▼                ▼
              Status           Control
```

## Features

- C++17 and Linux/POSIX sockets
- Concurrent device simulation and telemetry collection
- Thread-safe producer/consumer queue using `mutex` and `condition_variable`
- SQLite persistence with prepared statements
- HTTP API for status, telemetry history, restart, and fan control
- Graceful shutdown using SIGINT/SIGTERM
- Unit test for the telemetry queue
- CMake build and CTest integration

## Build

Ubuntu/Debian:

```bash
sudo apt update
sudo apt install build-essential cmake libsqlite3-dev

cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Run

```bash
./build/edge_server
```

Optional port:

```bash
./build/edge_server 9090
```

## API examples

Get device status:

```bash
curl http://127.0.0.1:8080/api/device/status
```

Get latest telemetry:

```bash
curl http://127.0.0.1:8080/api/telemetry
```

Set fan speed:

```bash
curl -X POST http://127.0.0.1:8080/api/device/fan \
  -H 'Content-Type: application/json' \
  -d '{"rpm":2500}'
```

Restart device state:

```bash
curl -X POST http://127.0.0.1:8080/api/device/restart
```

## Design decisions

### Producer/consumer telemetry pipeline

The simulated device is sampled periodically and records are placed into a thread-safe queue. A separate writer thread consumes records and writes them to SQLite. This prevents database I/O from blocking the sensor loop.

### Database safety

SQLite prepared statements are used for inserts and queries. A composite index on `(device_id, timestamp)` supports recent-device telemetry queries.

### Graceful shutdown

SIGINT/SIGTERM sets a stop flag. The HTTP server, device thread, collector, queue, and database writer are then stopped and joined in an orderly sequence.

## Suggested extensions

- Add device authentication/API keys
- Add bounded queue/backpressure
- Add configuration file support
- Add HTTP request parsing with Content-Length
- Add JSON library such as nlohmann/json
- Add Prometheus metrics
- Add integration tests
- Add real serial/I2C sensor abstraction behind an interface
- Add OTA update simulation

## Resume bullets

- Built a Linux-based C++17 embedded device telemetry and control system with concurrent sensor simulation, thread-safe producer/consumer processing, and graceful shutdown.
- Implemented SQLite-backed telemetry persistence and HTTP APIs for device status, historical telemetry, fan-speed control, and device restart operations.
- Added CMake/CTest-based automated testing, prepared SQL statements, structured project documentation, and POSIX socket-based client communication.
