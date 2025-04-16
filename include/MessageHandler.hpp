#pragma once
#include <string>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <coroutine>

struct ChatMessage {
    enum class Sender { User, AI };
    Sender sender;
    std::string content;
};

class MessageHandler {
public:
    void append_to_last_ai_message(const std::string& chunk, bool is_complete = false);
    void log_complete_ai_message();

    MessageHandler();
    void push_message(const ChatMessage& msg);
    std::vector<ChatMessage> get_messages(int offset = 0, int count = 50) const;
    int message_count() const;
    void clear();
private:
    mutable std::mutex mutex_;
    std::deque<ChatMessage> messages_;
};
