#pragma once
#include "BaseAIClient.hpp"

class OpenAIClient : public BaseAIClient {
public:
    OpenAIClient() { model_ = "gpt-4o"; }
    
    nlohmann::json build_message_history(const std::string& latest_user_msg = "") const override {
        std::lock_guard lock(mutex_);
        nlohmann::json messages = nlohmann::json::array();
        
        // Add system message if available
        if (!system_prompt_.empty()) {
            messages.push_back({{"role", "system"}, {"content", system_prompt_}});
        }
        
        // Add conversation history
        for (const auto& msg : conversation_history_) {
            messages.push_back(msg);
        }
        
        // Add latest user message if provided
        if (!latest_user_msg.empty()) {
            messages.push_back({{"role", "user"}, {"content", latest_user_msg}});
        }
        
        return messages;
    }
    
    std::future<std::expected<std::string, ApiErrorInfo>> send_message(
        const nlohmann::json& messages, 
        const std::string& model = "") override;
};