#pragma once
#include <format>
#include <string>
#include <iostream>
#include <fstream>
#include <memory>
#include <mutex>
#include <source_location>
#include <chrono>
#include <ctime>
#include <nlohmann/json.hpp>

enum class LogLevel { Debug, Info, Warning, Error, Critical };

class RichLogger {
public:
    RichLogger(const std::string& filename = "", bool json_mode = false)
        : file_(filename.empty() ? nullptr : new std::ofstream(filename, std::ios::app)),
          json_mode_(json_mode) {}

    void set_level(LogLevel level) { level_ = level; }
    void set_json_mode(bool json_mode) { json_mode_ = json_mode; }

    void log(LogLevel level, const std::string& msg,
             const std::source_location& loc = std::source_location::current()) {
        if (level < level_) return;
        std::lock_guard lock(mutex_);
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        char timebuf[32];
        std::strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", std::localtime(&now_c));
        if (json_mode_) {
            nlohmann::json j = {
                {"timestamp", timebuf},
                {"level", level_to_string(level)},
                {"file", loc.file_name()},
                {"line", loc.line()},
                {"function", loc.function_name()},
                {"message", msg}
            };
            std::string output = j.dump();
            if (!file_) std::cout << output << std::endl;
            else *file_ << output << std::endl;
        } else {
            std::string output = std::format("[{}] {} {}:{} {}() | {}\n",
                timebuf, level_to_string(level), loc.file_name(), loc.line(), loc.function_name(), msg);
            if (!file_) {
                std::cout << color_for(level) << output << "\033[0m";
            } else {
                *file_ << output;
            }
        }
    }
private:
    std::unique_ptr<std::ofstream> file_;
    std::mutex mutex_;
    LogLevel level_ = LogLevel::Debug;
    bool json_mode_ = false;
    static std::string level_to_string(LogLevel level) {
        switch (level) {
            case LogLevel::Debug: return "DEBUG";
            case LogLevel::Info: return "INFO";
            case LogLevel::Warning: return "WARN";
            case LogLevel::Error: return "ERROR";
            case LogLevel::Critical: return "CRIT";
        }
        return "UNK";
    }
    static const char* color_for(LogLevel level) {
        switch (level) {
            case LogLevel::Debug: return "\033[36m";      // Cyan
            case LogLevel::Info: return "\033[32m";       // Green
            case LogLevel::Warning: return "\033[33m";    // Yellow
            case LogLevel::Error: return "\033[31m";      // Red
            case LogLevel::Critical: return "\033[41;37m";// White on Red
        }
        return "";
    }
};
