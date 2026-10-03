#include "logger.h"
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>

void logMessage(LogLevel level, const std::string& message) {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now{};
    localtime_r(&now_c, &tm_now); // Thread-safe local time

    const char* tag = "INFO";
    if (level == LogLevel::WARN)  tag = "WARN";
    if (level == LogLevel::ERROR) tag = "ERROR";

    std::ostream& out = (level == LogLevel::ERROR) ? std::cerr : std::cout;
    out << "[" << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S") << "] "
        << "[" << tag << "] "
        << message << "\n";
}