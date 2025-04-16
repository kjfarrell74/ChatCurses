#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <format>

class Logger {
public:
    enum class Level { Debug, Info, Warning, Error };
    Logger(const std::string& filename = "chatbot.log");
    void log(Level level, const std::string& msg);
    // Use LOGF macro for formatted logging
private:
    std::ofstream ofs_;

// Macro for formatted logging
#ifndef LOGGER_LOGF_MACRO
#define LOGGER_LOGF_MACRO
#define LOGF(logger, level, fmt, ...) \
    (logger).log((level), std::format((fmt), __VA_ARGS__))
#endif
    std::mutex mutex_;
};
