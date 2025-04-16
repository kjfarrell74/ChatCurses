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

std::future<std::expected<std::string, ApiErrorInfo>> ClaudeAIClient::send_message(const nlohmann::json& messages, const std::string& model) {
    return std::async(std::launch::async, [this, messages, model]() -> std::expected<std::string, ApiErrorInfo> {
        if (api_key_.empty()) {
            return std::unexpected(ApiErrorInfo{.code = ApiError::ApiKeyNotSet, .message = "API key is required but not set."});
        }
        CURL* curl = curl_easy_init();
        if (!curl) {
            return std::unexpected(ApiErrorInfo{.code = ApiError::CurlInitFailed, .message = "Failed to initialize CURL handle."});
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
        
        std::string model_to_use = model.empty() ? model_ : model;
        
        nlohmann::json req = {
            {"model", model_to_use},
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
            return std::unexpected(ApiErrorInfo{.code = ApiError::NetworkError, .message = curl_easy_strerror(res)});
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
                return std::unexpected(ApiErrorInfo{.code = ApiError::MalformedResponse, .message = resp["error"].dump()});
            } else {
                return std::unexpected(ApiErrorInfo{.code = ApiError::MalformedResponse, .message = "Malformed response"});
            }
        } catch (const std::exception& e) {
            return std::unexpected(ApiErrorInfo{.code = ApiError::JsonParseError, .message = e.what()});
        }
    });
}