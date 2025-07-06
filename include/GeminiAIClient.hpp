#pragma once
#include "BaseAIClient.hpp"
#include <string>
#include <mutex>
#include <future>
#include <expected>
#include <nlohmann/json.hpp>

class GeminiAIClient : public BaseAIClient {
public:
    GeminiAIClient();
    ~GeminiAIClient() override = default;

    nlohmann::json build_message_history(const std::string& latest_user_msg = "") const override;

    std::future<std::expected<std::string, ApiErrorInfo>> send_message(
        const nlohmann::json& messages, 
        const std::string& model = "") override;

    void send_message_stream(
        const std::string& prompt,
        const std::string& model,
        std::function<void(const std::string& chunk, bool is_last_chunk)> on_chunk_cb,
        std::function<void()> on_done_cb,
        std::function<void(const ApiErrorInfo& error)> on_error_cb) override;

private:
    static const std::string BASE_URL;
    static const std::string API_VERSION;
    
    std::string build_request_url(const std::string& model) const;
    nlohmann::json build_request_body(const nlohmann::json& messages) const;
    std::expected<std::string, ApiErrorInfo> parse_response(const std::string& response) const;
    std::expected<std::string, ApiErrorInfo> make_api_request(const std::string& url, const nlohmann::json& request_body) const;
};