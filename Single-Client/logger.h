#pragma once
#include <string>

enum class LogLevel {
    INFO,
    WARN,
    ERROR
};

// Logs formatted as: [YYYY-MM-DD HH:MM:SS] message
void logMessage(LogLevel level, const std::string& message);
