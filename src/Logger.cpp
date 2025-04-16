#include "Logger.hpp"
#include <chrono>
#include <ctime>

Logger::Logger(const std::string& filename) : ofs_(filename, std::ios::app) {}

void Logger::log(Level level, const std::string& msg) {
    std::lock_guard lock(mutex_);
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    ofs_ << std::put_time(std::localtime(&now), "%F %T") << " [";
    switch (level) {
        case Level::Debug: ofs_ << "DEBUG"; break;
        case Level::Info: ofs_ << "INFO"; break;
        case Level::Warning: ofs_ << "WARN"; break;
        case Level::Error: ofs_ << "ERROR"; break;
    }
    ofs_ << "] " << msg << std::endl;
}
