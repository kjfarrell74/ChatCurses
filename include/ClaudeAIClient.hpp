#pragma once
#include "BaseAIClient.hpp"

class ClaudeAIClient : public BaseAIClient {
public:
    ClaudeAIClient() { model_ = "claude"; }
    
    nlohmann::json build_message_history(const std::string& latest_user_msg = "") const override {
        std::lock_guard lock(mutex_);
        nlohmann::json messages = nlohmann::json::array();
        for (const auto& msg : conversation_history_) {
            if (msg.contains("role") && msg["role"] == "system") continue;
            messages.push_back(msg);
        }
        if (!latest_user_msg.empty()) {
            messages.push_back({{"role", "user"}, {"content", latest_user_msg}});
        }
        return messages;
    }
    
    std::future<std::expected<std::string, ApiErrorInfo>> send_message(
        const nlohmann::json& messages, 
        const std::string& model = "") override;
};