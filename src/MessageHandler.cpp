#include "MessageHandler.hpp"
#include <fstream>

MessageHandler::MessageHandler() {}

void MessageHandler::push_message(const ChatMessage& msg) {
    std::lock_guard lock(mutex_);
    messages_.push_back(msg);
    // Log to chat_history.log
    std::ofstream log("chat_history.log", std::ios::app);
    if (log) {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        char timebuf[32];
        std::strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", std::localtime(&now_c));
        const char* sender = (msg.sender == ChatMessage::Sender::User) ? "USER" : "AI";
        log << "[" << timebuf << "] [" << sender << "] " << msg.content << std::endl;
    }
}

std::vector<ChatMessage> MessageHandler::get_messages(int offset, int count) const {
    std::lock_guard lock(mutex_);
    std::vector<ChatMessage> result;
    int start = std::max(0, (int)messages_.size() - offset - count);
    int end = std::max(0, (int)messages_.size() - offset);
    for (int i = start; i < end; ++i) {
        result.push_back(messages_[i]);
    }
    return result;
}

int MessageHandler::message_count() const {
    std::lock_guard lock(mutex_);
    return messages_.size();
}

void MessageHandler::append_to_last_ai_message(const std::string& text, bool is_complete) {
    std::lock_guard lock(mutex_);
    if (!messages_.empty() && messages_.back().sender == ChatMessage::Sender::AI) {
        messages_.back().content += text;
        // Log every chunk append
        std::ofstream log("chat_history.log", std::ios::app);
        if (log) {
            auto now = std::chrono::system_clock::now();
            std::time_t now_c = std::chrono::system_clock::to_time_t(now);
            char timebuf[32];
            std::strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", std::localtime(&now_c));
            log << "[" << timebuf << "] [AI] (chunk append) " << text << std::endl;
            
            // If this is the last chunk, log the complete message
            if (is_complete) {
                log_complete_ai_message();
            }
        }
    }
}

void MessageHandler::log_complete_ai_message() {
    if (!messages_.empty() && messages_.back().sender == ChatMessage::Sender::AI) {
        std::ofstream log("chat_history.log", std::ios::app);
        if (log) {
            auto now = std::chrono::system_clock::now();
            std::time_t now_c = std::chrono::system_clock::to_time_t(now);
            char timebuf[32];
            std::strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", std::localtime(&now_c));
            log << "[" << timebuf << "] [AI] (complete message) " << messages_.back().content << std::endl;
        }
    }
}

void MessageHandler::clear() {
    std::lock_guard lock(mutex_);
    messages_.clear();
}
