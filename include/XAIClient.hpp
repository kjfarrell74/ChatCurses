#pragma once
#include "BaseAIClient.hpp"
#include <concepts>
#include <coroutine>

// Concept for AI provider extensibility
template<typename T>
concept AIProvider = requires(T a, const std::string& prompt, const std::string& model) {
    { a.send_message(prompt, model) } -> std::same_as<std::future<std::string>>;
    { a.set_api_key(std::string{}) };
    { a.set_system_prompt(std::string{}) };
    { a.set_model(std::string{}) };
};

class XAIClient : public BaseAIClient {
public:
    XAIClient() = default;
    
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
    
    // Override the send_message method to handle xAI-specific implementation
    std::future<std::expected<std::string, ApiErrorInfo>> send_message(
        const nlohmann::json& messages, 
        const std::string& model = "") override;
    
    // Legacy method signature - delegates to the interface method
    std::future<std::expected<std::string, ApiErrorInfo>> send_message(
        const std::string& prompt,
        const std::string& model = "");
    
    // Override the streaming interface with xAI-specific implementation
    void send_message_stream(
        const std::string& prompt,
        const std::string& model,
        std::function<void(const std::string& chunk, bool is_last_chunk)> on_chunk_cb,
        std::function<void()> on_done_cb,
        std::function<void(const ApiErrorInfo& error)> on_error_cb) override;
    
    std::vector<std::string> available_models() const;
};