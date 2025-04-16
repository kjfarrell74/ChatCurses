#pragma once
#include <memory>
#include <atomic>

class ChatbotAppImpl; // Forward declaration

class ChatbotAppImpl; // Forward declaration

class ChatbotApp {
public:
    ChatbotApp();
    void run();
    ~ChatbotApp();
private:
    std::unique_ptr<ChatbotAppImpl> impl_;
};
