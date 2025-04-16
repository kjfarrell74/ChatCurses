#pragma once
#include <string>
#include <future>
#include <vector>
#include <mutex>
#include <nlohmann/json.hpp>
#include <expected>
#include "AICommon.hpp"

class AIClientInterface {
public:
    virtual ~AIClientInterface() = default;
    
    // Common configuration methods
    virtual void set_api_key(const std::string& key) = 0;
    virtual void set_system_prompt(const std::string& prompt) = 0;
    virtual void set_model(const std::string& model) = 0;
    
    // Conversation history management
    virtual void clear_history() = 0;
    virtual void push_user_message(const std::string& content) = 0;
    virtual void push_assistant_message(const std::string& content) = 0;
    virtual nlohmann::json build_message_history(const std::string& latest_user_msg = "") const = 0;
    
    // Core messaging functionality
    virtual std::future<std::expected<std::string, ApiErrorInfo>> send_message(
        const nlohmann::json& messages, 
        const std::string& model = "") = 0;
    
    // Optional streaming interface
    virtual void send_message_stream(
        const std::string& prompt,
        const std::string& model,
        std::function<void(const std::string& chunk, bool is_last_chunk)> on_chunk_cb,
        std::function<void()> on_done_cb,
        std::function<void(const ApiErrorInfo& error)> on_error_cb) {
        // Default implementation uses the send_message method
        std::thread([this, prompt, model, on_chunk_cb, on_done_cb, on_error_cb]() {
            auto messages = this->build_message_history(prompt);
            auto fut = this->send_message(messages, model);
            auto result = fut.get();
            if (result) {
                on_chunk_cb(*result, true);
                on_done_cb();
            } else {
                on_error_cb(result.error());
            }
        }).detach();
    }
};