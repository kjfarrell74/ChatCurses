#include "GeminiAIClient.hpp"
#include "GlobalLogger.hpp"
#include <format>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <regex>

const std::string GeminiAIClient::BASE_URL = "https://generativelanguage.googleapis.com";
const std::string GeminiAIClient::API_VERSION = "v1beta";

size_t GeminiWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    static_cast<std::string*>(userp)->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

GeminiAIClient::GeminiAIClient() {
    model_ = "gemini-1.5-pro";
}

nlohmann::json GeminiAIClient::build_message_history(const std::string& latest_user_msg) const {
    std::lock_guard lock(mutex_);
    auto history = conversation_history_;
    if (!latest_user_msg.empty()) {
        history.push_back({{"role", "user"}, {"content", latest_user_msg}});
    }
    return history;
}

std::future<std::expected<std::string, ApiErrorInfo>> GeminiAIClient::send_message(
    const nlohmann::json& messages, 
    const std::string& model) {
    
    return std::async(std::launch::async, [this, messages, model]() -> std::expected<std::string, ApiErrorInfo> {
        std::lock_guard lock(mutex_);
        
        if (api_key_.empty()) {
            return std::unexpected(ApiErrorInfo{ApiError::ApiKeyNotSet, "API key not set"});
        }
        
        try {
            // Process with MCP tools if needed - check the last user message
            std::string tool_results = "";
            if (!messages.empty()) {
                auto last_message = messages.back();
                if (last_message.contains("content")) {
                    std::string user_message = last_message["content"];
                    tool_results = process_with_mcp_tools(user_message);
                }
            }
            
            // Enhanced system prompt with tools
            std::string enhanced_prompt = enhance_system_prompt_with_tools(system_prompt_);
            
            // Build request body
            nlohmann::json request_body = build_request_body(messages);
            
            // Modified messages with tool results if available
            if (!tool_results.empty() && !messages.empty()) {
                // Add tool results to the last user message
                auto& contents = request_body["contents"];
                if (!contents.empty()) {
                    auto& last_content = contents.back();
                    if (last_content.contains("parts") && !last_content["parts"].empty()) {
                        auto& last_part = last_content["parts"].back();
                        if (last_part.contains("text")) {
                            std::string original_text = last_part["text"];
                            last_part["text"] = original_text + "\n\n## Tool Results:\n" + tool_results;
                        }
                    }
                }
            }
            
            // Add system instruction if available
            if (!enhanced_prompt.empty()) {
                request_body["systemInstruction"] = {
                    {"parts", {{{"text", enhanced_prompt}}}}
                };
            }
            
            // Make API request
            std::string url = build_request_url(model.empty() ? model_ : model);
            auto response = make_api_request(url, request_body);
            
            if (!response) {
                return std::unexpected(response.error());
            }
            
            return parse_response(response.value());
            
        } catch (const std::exception& e) {
            get_logger().log(LogLevel::Error, std::format("GeminiAIClient::send_message exception: {}", e.what()));
            return std::unexpected(ApiErrorInfo{ApiError::CurlRequestFailed, std::string("Request failed: ") + e.what()});
        }
    });
}

void GeminiAIClient::send_message_stream(
    const std::string& prompt,
    const std::string& model,
    std::function<void(const std::string& chunk, bool is_last_chunk)> on_chunk_cb,
    std::function<void()> on_done_cb,
    std::function<void(const ApiErrorInfo& error)> on_error_cb) {
    
    // For now, use non-streaming approach
    // TODO: Implement proper streaming when needed
    std::thread([this, prompt, model, on_chunk_cb, on_done_cb, on_error_cb]() {
        nlohmann::json messages = nlohmann::json::array();
        messages.push_back({{"role", "user"}, {"content", prompt}});
        
        auto future_result = send_message(messages, model);
        auto result = future_result.get();
        
        if (result) {
            on_chunk_cb(result.value(), true);
            on_done_cb();
        } else {
            on_error_cb(result.error());
        }
    }).detach();
}

std::string GeminiAIClient::build_request_url(const std::string& model) const {
    return std::format("{}/{}/models/{}:generateContent?key={}", 
                      BASE_URL, API_VERSION, model, api_key_);
}

nlohmann::json GeminiAIClient::build_request_body(const nlohmann::json& messages) const {
    nlohmann::json request_body;
    nlohmann::json contents = nlohmann::json::array();
    
    for (const auto& message : messages) {
        if (!message.contains("role") || !message.contains("content")) {
            continue;
        }
        
        std::string role = message["role"];
        std::string content = message["content"];
        
        // Map roles to Gemini format
        if (role == "user") {
            contents.push_back({
                {"role", "user"},
                {"parts", {{{"text", content}}}}
            });
        } else if (role == "assistant") {
            contents.push_back({
                {"role", "model"},
                {"parts", {{{"text", content}}}}
            });
        }
    }
    
    request_body["contents"] = contents;
    request_body["generationConfig"] = {
        {"temperature", 0.7},
        {"topK", 40},
        {"topP", 0.95},
        {"maxOutputTokens", 4000}
    };
    
    return request_body;
}

std::expected<std::string, ApiErrorInfo> GeminiAIClient::parse_response(const std::string& response) const {
    try {
        auto json_response = nlohmann::json::parse(response);
        
        if (json_response.contains("error")) {
            auto error = json_response["error"];
            std::string error_message = error.contains("message") ? error["message"] : "Unknown error";
            return std::unexpected(ApiErrorInfo{ApiError::CurlRequestFailed, error_message});
        }
        
        if (json_response.contains("candidates") && !json_response["candidates"].empty()) {
            auto candidate = json_response["candidates"][0];
            if (candidate.contains("content") && candidate["content"].contains("parts")) {
                auto parts = candidate["content"]["parts"];
                if (!parts.empty() && parts[0].contains("text")) {
                    return parts[0]["text"].get<std::string>();
                }
            }
        }
        
        return std::unexpected(ApiErrorInfo{ApiError::InvalidResponse, "No valid response content found"});
        
    } catch (const std::exception& e) {
        get_logger().log(LogLevel::Error, std::format("GeminiAIClient::parse_response exception: {}", e.what()));
        return std::unexpected(ApiErrorInfo{ApiError::InvalidResponse, std::string("Failed to parse response: ") + e.what()});
    }
}

std::expected<std::string, ApiErrorInfo> GeminiAIClient::make_api_request(const std::string& url, const nlohmann::json& request_body) const {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return std::unexpected(ApiErrorInfo{ApiError::CurlInitFailed, "Failed to initialize curl"});
    }

    std::string response_data;
    std::string request_body_str = request_body.dump();
    
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body_str.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, GeminiWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "ChatCurses/1.0");
    
    get_logger().log(LogLevel::Debug, std::format("GeminiAIClient::make_api_request - URL: {}", url));
    get_logger().log(LogLevel::Debug, std::format("GeminiAIClient::make_api_request - Request: {}", request_body_str));
    
    CURLcode res = curl_easy_perform(curl);
    
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        std::string error_msg = std::format("curl_easy_perform() failed: {}", curl_easy_strerror(res));
        get_logger().log(LogLevel::Error, std::format("GeminiAIClient::make_api_request - {}", error_msg));
        return std::unexpected(ApiErrorInfo{ApiError::CurlRequestFailed, error_msg});
    }
    
    get_logger().log(LogLevel::Debug, std::format("GeminiAIClient::make_api_request - Response code: {}", response_code));
    get_logger().log(LogLevel::Debug, std::format("GeminiAIClient::make_api_request - Response: {}", response_data));
    
    if (response_code != 200) {
        std::string error_msg = std::format("HTTP error: {}", response_code);
        return std::unexpected(ApiErrorInfo{ApiError::CurlRequestFailed, error_msg});
    }
    
    return response_data;
}