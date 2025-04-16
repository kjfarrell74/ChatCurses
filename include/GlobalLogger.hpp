#pragma once
#include "RichLogger.hpp"

// Global singleton logger instance
inline RichLogger& get_logger() {
    static RichLogger logger{"chatbot.log"};
    return logger;
}
