#include "http_server.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>
#include <ctime>
#include <iomanip>

namespace {
std::string json_state(const DeviceState& s) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(2);
    out << "{\"device_id\":\"edge-01\",\"running\":" << (s.running ? "true" : "false")
        << ",\"temperature\":" << s.temperature
        << ",\"battery\":" << s.battery
        << ",\"cpu\":" << s.cpu
        << ",\"fan_speed\":" << s.fan_speed << "}";
    return out.str();
}
std::string json_history(const std::vector<TelemetryRecord>& rows) {
    std::ostringstream out;
    out << "[";
    for (size_t i = 0; i < rows.size(); ++i) {
        if (i) out << ',';
        out << "{\"device_id\":\"" << rows[i].device_id << "\",\"sensor\":\"" << rows[i].sensor
            << "\",\"value\":" << rows[i].value << ",\"timestamp\":" << rows[i].timestamp << "}";
    }
    out << "]";
    return out.str();
}
}

HttpServer::HttpServer(int port, DeviceSimulator& device, Database& db) : port_(port), device_(device), db_(db) {}
HttpServer::~HttpServer() { stop(); }

bool HttpServer::start() {
    if (running_) return true;
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) return false;
    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(static_cast<uint16_t>(port_));
    if (bind(server_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0 || listen(server_fd_, 16) < 0) {
        close(server_fd_); server_fd_ = -1; return false;
    }
    running_ = true;
    accept_thread_ = std::thread(&HttpServer::accept_loop, this);
    return true;
}

void HttpServer::stop() {
    if (!running_.exchange(false)) return;
    shutdown(server_fd_, SHUT_RDWR);
    close(server_fd_);
    server_fd_ = -1;
    if (accept_thread_.joinable()) accept_thread_.join();
}

void HttpServer::accept_loop() {
    while (running_) {
        int client = accept(server_fd_, nullptr, nullptr);
        if (client < 0) { if (running_) continue; break; }
        std::thread(&HttpServer::handle_client, this, client).detach();
    }
}

void HttpServer::handle_client(int fd) {
    char buffer[8192]{};
    const ssize_t n = recv(fd, buffer, sizeof(buffer) - 1, 0);
    if (n <= 0) { close(fd); return; }
    std::string request(buffer, static_cast<size_t>(n));
    std::istringstream in(request);
    std::string method, path, version;
    in >> method >> path >> version;
    std::string body;
    const auto body_pos = request.find("\r\n\r\n");
    if (body_pos != std::string::npos) body = request.substr(body_pos + 4);
    const std::string response_body = route(method, path, body);
    const bool ok = response_body.rfind("{\"error\"", 0) != 0;
    std::ostringstream response;
    response << "HTTP/1.1 " << (ok ? "200 OK" : "400 Bad Request") << "\r\n"
             << "Content-Type: application/json\r\nContent-Length: " << response_body.size()
             << "\r\nConnection: close\r\n\r\n" << response_body;
    const std::string out = response.str();
    send(fd, out.data(), out.size(), 0);
    close(fd);
}

std::string HttpServer::route(const std::string& method, const std::string& path, const std::string& body) {
    if (method == "GET" && path == "/api/device/status") return json_state(device_.get_state());
    if (method == "GET" && path == "/api/telemetry") return json_history(db_.latest("edge-01", 50));
    if (method == "POST" && path == "/api/device/restart") {
        device_.restart(); return "{\"status\":\"restarted\"}";
    }
    if (method == "POST" && path == "/api/device/fan") {
        const auto pos = body.find("rpm");
        if (pos == std::string::npos) return "{\"error\":\"body must contain rpm\"}";
        const auto colon = body.find(':', pos);
        if (colon == std::string::npos) return "{\"error\":\"invalid rpm\"}";
        try { device_.set_fan_speed(std::stod(body.substr(colon + 1))); }
        catch (...) { return "{\"error\":\"invalid rpm\"}"; }
        return "{\"status\":\"fan updated\"}";
    }
    return "{\"error\":\"unknown endpoint\"}";
}
