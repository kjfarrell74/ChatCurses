#include "OpenAIClient.hpp"
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

OpenAIClient::OpenAIClient() {}

void OpenAIClient::set_api_key(const std::string& key) {
    std::lock_guard lock(mutex_);
    api_key_ = key;
}

void OpenAIClient::set_system_prompt(const std::string& prompt) {
    std::lock_guard lock(mutex_);
    system_prompt_ = prompt;
}

void OpenAIClient::set_model(const std::string& model) {
    std::lock_guard lock(mutex_);
    model_ = model;
}

void OpenAIClient::clear_history() {
    std::lock_guard lock(mutex_);
    conversation_history_.clear();
}

void OpenAIClient::push_user_message(const std::string& content) {
    std::lock_guard lock(mutex_);
    conversation_history_.push_back({{"role", "user"}, {"content", content}});
}

void OpenAIClient::push_assistant_message(const std::string& content) {
    std::lock_guard lock(mutex_);
    conversation_history_.push_back({{"role", "assistant"}, {"content", content}});
}

nlohmann::json OpenAIClient::build_message_history(const std::string& latest_user_msg) const {
    std::lock_guard lock(mutex_);
    nlohmann::json messages = nlohmann::json::array();
    if (!system_prompt_.empty()) {
        messages.push_back({{"role", "system"}, {"content", system_prompt_}});
    }
    for (const auto& msg : conversation_history_) {
        messages.push_back(msg);
    }
    if (!latest_user_msg.empty()) {
        messages.push_back({{"role", "user"}, {"content", latest_user_msg}});
    }
    return messages;
}

std::future<std::expected<std::string, ApiErrorInfo>> OpenAIClient::send_message(const nlohmann::json& messages, const std::string& model) {
    return std::async(std::launch::async, [this, messages, model]() -> std::expected<std::string, ApiErrorInfo> {
        if (api_key_.empty()) {
            return std::unexpected(ApiErrorInfo{.error = ApiError::ApiKeyNotSet, .message = "API key is required but not set."});
        }
        CURL* curl = curl_easy_init();
        if (!curl) {
            return std::unexpected(ApiErrorInfo{.error = ApiError::CurlInitFailed, .message = "Failed to initialize CURL handle."});
        }
        auto curl_cleanup = [](CURL* c){ if(c) curl_easy_cleanup(c); };
        std::unique_ptr<CURL, decltype(curl_cleanup)> curl_ptr(curl, curl_cleanup);
        auto slist_cleanup = [](curl_slist* s){ if(s) curl_slist_free_all(s); };
        std::unique_ptr<curl_slist, decltype(slist_cleanup)> headers_ptr(nullptr, slist_cleanup);
        std::string readBuffer;
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("Authorization: Bearer " + api_key_).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers_ptr.reset(headers);
        nlohmann::json req = {
            {"model", model.empty() ? model_ : model},
            {"messages", messages},
            {"max_tokens", 1024}
        };
        std::string req_str = req.dump();
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.openai.com/v1/chat/completions");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req_str.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            return std::unexpected(ApiErrorInfo{.error = ApiError::CurlRequestFailed, .message = curl_easy_strerror(res)});
        }
        try {
            auto resp = nlohmann::json::parse(readBuffer);
            if (resp.contains("choices") && resp["choices"].is_array() && !resp["choices"].empty()) {
                auto& msg = resp["choices"][0]["message"];
                if (msg.contains("content")) {
                    return msg["content"].get<std::string>();
                }
            } else if (resp.contains("error")) {
                return std::unexpected(ApiErrorInfo{.error = ApiError::MalformedResponse, .message = resp["error"].dump()});
            }
            return std::unexpected(ApiErrorInfo{.error = ApiError::MalformedResponse, .message = "Malformed response"});
        } catch (const std::exception& e) {
            return std::unexpected(ApiErrorInfo{.error = ApiError::MalformedResponse, .message = e.what()});
        }
    });
}
