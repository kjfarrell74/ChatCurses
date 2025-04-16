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
    std::function<void(const std::string& chunk, bool is_last_chunk)> on_chunk,
    std::function<void()> on_done)
{
    std::thread([=]() {
        std::string response = this->send_message(prompt, model).get();
        size_t pos = 0;
        const size_t min_chunk_size = 40;
        while (pos < response.size()) {
            // Find a chunk boundary at a space or end of string, but not splitting a UTF-8 character
            size_t chunk_end = pos + min_chunk_size;
            if (chunk_end >= response.size()) chunk_end = response.size();
            else {
                // Backtrack to last space (word boundary) within chunk
                size_t space = response.rfind(' ', chunk_end);
                if (space != std::string::npos && space > pos) chunk_end = space + 1;
                // Backtrack to valid UTF-8 boundary
                while (chunk_end > pos && (response[chunk_end] & 0xC0) == 0x80) --chunk_end;
                if (chunk_end == pos) chunk_end = pos + min_chunk_size; // fallback: force progress
            }
            std::string chunk = response.substr(pos, chunk_end - pos);
            bool is_last_chunk = (chunk_end >= response.size());
            on_chunk(chunk, is_last_chunk);
            pos = chunk_end;
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
        }
        on_done();
    }).detach();
}

std::future<std::string> XAIClient::send_message(const std::string& prompt, const std::string& model) {
    // Placeholder: simulate async xAI API call
    return std::async(std::launch::async, [this, prompt, model]() -> std::string {
        if (api_key_.empty()) {
            last_error_ = "API key not set";
            return "[Error: API key not set]";
        }
        CURL* curl = curl_easy_init();
        if (!curl) {
            last_error_ = "Failed to initialize CURL";
            return "[Error: CURL init failed]";
        }
        std::string readBuffer;
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("Authorization: Bearer " + api_key_).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");

        nlohmann::json req = {
            {"model", model.empty() ? "grok-3-beta" : model},
            {"messages", {
                { {"role", "system"}, {"content", system_prompt_.empty() ? "You are a chatbot." : system_prompt_} },
                { {"role", "user"}, {"content", prompt} }
            }},
            {"max_tokens", 256}
        };
        std::string req_str = req.dump();

        curl_easy_setopt(curl, CURLOPT_URL, "https://api.x.ai/v1/chat/completions");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req_str.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        CURLcode res = curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            last_error_ = curl_easy_strerror(res);
            return std::string("[Error: ") + last_error_ + "]";
        }

        try {
            auto resp = nlohmann::json::parse(readBuffer);
            if (resp.contains("choices") && !resp["choices"].empty() && resp["choices"][0].contains("message") && resp["choices"][0]["message"].contains("content")) {
                std::string raw = resp["choices"][0]["message"]["content"].get<std::string>();
                // Sanitize: remove non-printable and invalid UTF-8 chars
                std::string clean;
                size_t i = 0;
                while (i < raw.size()) {
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
                        // Skip invalid byte
                        ++i;
                    }
                }
                return clean;
            } else if (resp.contains("error")) {
                last_error_ = resp["error"].dump();
                return "[Error: " + last_error_ + "]";
            } else {
                last_error_ = "Malformed response";
                return "[Error: Malformed response]";
            }
        } catch (const std::exception& e) {
            last_error_ = e.what();
            return "[Error: " + last_error_ + "]";
        }
    });
}

std::vector<std::string> XAIClient::available_models() const {
    // Placeholder for available xAI models
    return {"xai-default", "xai-advanced"};
}

std::string XAIClient::last_error() const {
    return last_error_;
}
