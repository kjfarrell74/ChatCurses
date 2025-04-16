#pragma once
#include <nlohmann/json.hpp>
#include <future>
#include <mutex>
#include <string>
#include <vector>
#include <expected>
#include "AICommon.hpp"


class OpenAIClient {
public:
    OpenAIClient();
    void set_api_key(const std::string& key);
    void set_system_prompt(const std::string& prompt);
    void set_model(const std::string& model);
    void clear_history();
    void push_user_message(const std::string& content);
    void push_assistant_message(const std::string& content);
    nlohmann::json build_message_history(const std::string& latest_user_msg) const;
    std::future<std::expected<std::string, ApiErrorInfo>> send_message(const nlohmann::json& messages, const std::string& model = "");
private:
    mutable std::mutex mutex_;
    std::string api_key_;
    std::string system_prompt_;
    std::string model_ = "gpt-4o";
    std::vector<nlohmann::json> conversation_history_;
};
