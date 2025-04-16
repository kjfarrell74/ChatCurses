#include "XAIClient.hpp"
#include <future>
#include <thread>
#include <stdexcept>
#include <vector>
#include <string>
#include <cstring>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

XAIClient::XAIClient() {}

void XAIClient::set_api_key(const std::string& key) {
    api_key_ = key;
}

void XAIClient::set_system_prompt(const std::string& prompt) {
    system_prompt_ = prompt;
}

void XAIClient::set_model(const std::string& model) {
    model_ = model;
}

void XAIClient::send_message_stream(
    const std::string& prompt,
    const std::string& model,
    std::function<void(const std::string& chunk, bool is_last_chunk)> on_chunk_cb,
    std::function<void()> on_done_cb,
    std::function<void(const ApiErrorInfo& error)> on_error_cb)
{
    std::jthread([this, prompt, model, on_chunk_cb, on_done_cb, on_error_cb]() {
        std::future<std::expected<std::string, ApiErrorInfo>> future_result = this->send_message(prompt, model);
        std::expected<std::string, ApiErrorInfo> result = future_result.get();
        if (result) {
            const std::string& response = *result;
            size_t pos = 0;
            const size_t min_chunk_size = 40;
            while (pos < response.size()) {
                size_t chunk_end = pos + min_chunk_size;
                if (chunk_end >= response.size()) {
                    chunk_end = response.size();
                } else {
                    size_t space = response.rfind(' ', chunk_end);
                    if (space != std::string::npos && space > pos) {
                        chunk_end = space + 1;
                    }
                }
                std::string chunk = response.substr(pos, chunk_end - pos);
                bool is_last_chunk = (chunk_end >= response.size());
                on_chunk_cb(chunk, is_last_chunk);
                pos = chunk_end;
                std::this_thread::sleep_for(std::chrono::milliseconds(40));
            }
            on_done_cb();
        } else {
            on_error_cb(result.error());
        }
    }).detach();
}

std::future<std::expected<std::string, ApiErrorInfo>> XAIClient::send_message(const std::string& prompt, const std::string& model) {
    return std::async(std::launch::async, [this, prompt, model]() -> std::expected<std::string, ApiErrorInfo> {
        if (api_key_.empty()) {
            return std::unexpected(ApiErrorInfo{ApiError::ApiKeyNotSet, "API key is required but not set."});
        }
        CURL* curl = curl_easy_init();
        if (!curl) {
            return std::unexpected(ApiErrorInfo{ApiError::CurlInitFailed, "Failed to initialize CURL handle."});
        }
        // RAII for curl handle and slist
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
            {"model", model.empty() ? "grok-3-beta" : model},
            {"messages", nlohmann::json::array({ { {"role", "user"}, {"content", prompt} } })}
        };
        std::string req_str = req.dump();

        curl_easy_setopt(curl, CURLOPT_URL, "https://api.xai.com/v1/chat/completions");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req_str.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            return std::unexpected(ApiErrorInfo{ApiError::NetworkError, curl_easy_strerror(res)});
        }
        try {
            auto resp = nlohmann::json::parse(readBuffer);
            if (resp.contains("choices") && resp["choices"].is_array() && !resp["choices"].empty()) {
                std::string raw = resp["choices"][0]["message"]["content"].get<std::string>();
                std::string clean;
                for (size_t i = 0; i < raw.size();) {
                    unsigned char c = raw[i];
                    if (c >= 32 && c < 127) {
                        clean += c;
                        ++i;
                    } else if ((c & 0xE0) == 0xC0 && i+1 < raw.size() && (raw[i+1] & 0xC0) == 0x80) {
                        clean += raw.substr(i, 2);
                        i += 2;
                    } else if ((c & 0xF0) == 0xE0 && i+2 < raw.size() && (raw[i+1] & 0xC0) == 0x80 && (raw[i+2] & 0xC0) == 0x80) {
                        clean += raw.substr(i, 3);
                        i += 3;
                    } else if ((c & 0xF8) == 0xF0 && i+3 < raw.size() && (raw[i+1] & 0xC0) == 0x80 && (raw[i+2] & 0xC0) == 0x80 && (raw[i+3] & 0xC0) == 0x80) {
                        clean += raw.substr(i, 4);
                        i += 4;
                    } else {
                        ++i;
                    }
                }
                return clean;
            } else if (resp.contains("error")) {
                return std::unexpected(ApiErrorInfo{ApiError::MalformedResponse, resp["error"].dump()});
            } else {
                return std::unexpected(ApiErrorInfo{ApiError::MalformedResponse, "Malformed response"});
            }
        } catch (const std::exception& e) {
            return std::unexpected(ApiErrorInfo{ApiError::JsonParseError, e.what()});
        }
    });
}

std::vector<std::string> XAIClient::available_models() const {
    // Placeholder for available xAI models
    return {"xai-default", "xai-advanced"};
}
