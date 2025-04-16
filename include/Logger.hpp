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
    template<typename... Args>
    void logf(Level level, std::string_view fmt, Args&&... args) {
        log(level, std::format(fmt, std::forward<Args>(args)...));
    }
private:
    std::ofstream ofs_;
    std::mutex mutex_;
};
