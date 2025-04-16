#pragma once
#include <string>
#include <future>
#include <vector>
#include <mutex>
#include <nlohmann/json.hpp>
#include <expected>
#include "XAIClient.hpp" // for ApiErrorInfo

class ClaudeAIClient {
public:
    ClaudeAIClient();
    void set_api_key(const std::string& key);
    void set_system_prompt(const std::string& prompt);
    void set_model(const std::string& model);
    std::future<std::expected<std::string, ApiErrorInfo>> send_message(const nlohmann::json& messages, const std::string& model = "claude");
    void clear_history();
    void push_user_message(const std::string& content);
    void push_assistant_message(const std::string& content);
    nlohmann::json build_message_history(const std::string& latest_user_msg = "") const;
private:
    std::string api_key_;
    std::string system_prompt_;
    std::string model_ = "claude";
    mutable std::mutex mutex_;
    std::vector<nlohmann::json> conversation_history_;
};
