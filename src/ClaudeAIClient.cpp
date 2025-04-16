#include "ClaudeAIClient.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <future>
#include <memory>
#include <stdexcept>
#include <string>

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

ClaudeAIClient::ClaudeAIClient() {}

void ClaudeAIClient::set_api_key(const std::string& key) {
    std::lock_guard lock(mutex_);
    api_key_ = key;
}

void ClaudeAIClient::set_system_prompt(const std::string& prompt) {
    std::lock_guard lock(mutex_);
    system_prompt_ = prompt;
}

void ClaudeAIClient::set_model(const std::string& model) {
    std::lock_guard lock(mutex_);
    model_ = model;
}

void ClaudeAIClient::clear_history() {
    std::lock_guard lock(mutex_);
    conversation_history_.clear();
}

void ClaudeAIClient::push_user_message(const std::string& content) {
    std::lock_guard lock(mutex_);
    conversation_history_.push_back({{"role", "user"}, {"content", content}});
}

void ClaudeAIClient::push_assistant_message(const std::string& content) {
    std::lock_guard lock(mutex_);
    conversation_history_.push_back({{"role", "assistant"}, {"content", content}});
}

nlohmann::json ClaudeAIClient::build_message_history(const std::string& latest_user_msg) const {
    std::lock_guard lock(mutex_);
    nlohmann::json messages = nlohmann::json::array();
    for (const auto& msg : conversation_history_) {
        if (msg.contains("role") && msg["role"] == "system") continue; // skip any legacy system messages
        messages.push_back(msg);
    }
    if (!latest_user_msg.empty()) {
        messages.push_back({{"role", "user"}, {"content", latest_user_msg}});
    }
    return messages;
}

std::future<std::expected<std::string, ApiErrorInfo>> ClaudeAIClient::send_message(const nlohmann::json& messages, const std::string& model) {
    return std::async(std::launch::async, [this, messages, model]() -> std::expected<std::string, ApiErrorInfo> {
        if (api_key_.empty()) {
            return std::unexpected(ApiErrorInfo{.code = ApiError::ApiKeyNotSet, .details = "API key is required but not set."});
        }
        CURL* curl = curl_easy_init();
        if (!curl) {
            return std::unexpected(ApiErrorInfo{.code = ApiError::CurlInitFailed, .details = "Failed to initialize CURL handle."});
        }
        auto curl_cleanup = [](CURL* c){ if(c) curl_easy_cleanup(c); };
        std::unique_ptr<CURL, decltype(curl_cleanup)> curl_ptr(curl, curl_cleanup);
        auto slist_cleanup = [](curl_slist* s){ if(s) curl_slist_free_all(s); };
        std::unique_ptr<curl_slist, decltype(slist_cleanup)> headers_ptr(nullptr, slist_cleanup);
        std::string readBuffer;
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("x-api-key: " + api_key_).c_str());
        headers = curl_slist_append(headers, "anthropic-version: 2023-06-01");
        headers = curl_slist_append(headers, "content-type: application/json");
        headers_ptr.reset(headers);
        nlohmann::json req = {
            {"model", model.empty() ? "claude" : model},
            {"max_tokens", 1024},
            {"messages", messages}
        };
        if (!system_prompt_.empty()) {
            req["system"] = system_prompt_;
        }
        std::string req_str = req.dump();
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.anthropic.com/v1/messages");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req_str.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            return std::unexpected(ApiErrorInfo{.code = ApiError::NetworkError, .details = curl_easy_strerror(res)});
        }
        try {
            auto resp = nlohmann::json::parse(readBuffer);
            if (resp.contains("content") && resp["content"].is_array() && !resp["content"].empty()) {
                std::string reply;
                for (const auto& block : resp["content"]) {
                    if (block.contains("text")) {
                        reply += block["text"].get<std::string>();
                    }
                }
                return reply;
            } else if (resp.contains("error")) {
                return std::unexpected(ApiErrorInfo{.code = ApiError::MalformedResponse, .details = resp["error"].dump()});
            } else {
                return std::unexpected(ApiErrorInfo{.code = ApiError::MalformedResponse, .details = "Malformed response"});
            }
        } catch (const std::exception& e) {
            return std::unexpected(ApiErrorInfo{.code = ApiError::JsonParseError, .details = e.what()});
        }
    });
}
