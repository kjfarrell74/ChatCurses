#include "ClaudeAIClient.hpp"
#include "GlobalLogger.hpp"
#include <format>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <regex>

size_t ClaudeWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    static_cast<std::string*>(userp)->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

std::future<std::expected<std::string, ApiErrorInfo>> ClaudeAIClient::send_message(
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
            nlohmann::json request_body;
            request_body["model"] = model.empty() ? model_ : model;
            request_body["max_tokens"] = 4000;
            
            // Modified messages with tool results if available
            nlohmann::json modified_messages = messages;
            if (!tool_results.empty() && !messages.empty()) {
                // Modify the last user message to include tool results
                auto& last_msg = modified_messages.back();
                if (last_msg.contains("content")) {
                    std::string original_content = last_msg["content"];
                    last_msg["content"] = "Here are the results from available tools:\n\n" + tool_results + "\n\nNow please respond to: " + original_content;
                }
            }
            
            request_body["messages"] = modified_messages;
            
            // Add system prompt if present
            if (!enhanced_prompt.empty()) {
                request_body["system"] = enhanced_prompt;
            }
            
            // Prepare HTTP request
            CURL* curl = curl_easy_init();
            if (!curl) {
                return std::unexpected(ApiErrorInfo{ApiError::CurlInitFailed, "Failed to initialize curl"});
            }
            
            std::string response_string;
            struct curl_slist* headers = nullptr;
            
            // Set headers
            std::string auth_header = "x-api-key: " + api_key_;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            headers = curl_slist_append(headers, auth_header.c_str());
            headers = curl_slist_append(headers, "anthropic-version: 2023-06-01");
            
            // Set curl options
            std::string json_data = request_body.dump();
            curl_easy_setopt(curl, CURLOPT_URL, "https://api.anthropic.com/v1/messages");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, json_data.length());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ClaudeWriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
            
            // Perform request
            CURLcode res = curl_easy_perform(curl);
            
            // Check for curl errors
            if (res != CURLE_OK) {
                curl_slist_free_all(headers);
                curl_easy_cleanup(curl);
                return std::unexpected(ApiErrorInfo{ApiError::CurlRequestFailed, std::format("Request failed: {}", curl_easy_strerror(res))});
            }
            
            // Check HTTP status
            long response_code;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            
            if (response_code != 200) {
                return std::unexpected(ApiErrorInfo{ApiError::NetworkError, std::format("HTTP error {}: {}", response_code, response_string)});
            }
            
            // Parse response
            auto response_json = nlohmann::json::parse(response_string);
            
            if (!response_json.contains("content") || response_json["content"].empty()) {
                return std::unexpected(ApiErrorInfo{ApiError::MalformedResponse, "Invalid response format"});
            }
            
            auto content_array = response_json["content"];
            std::string content = "";
            
            for (const auto& item : content_array) {
                if (item.contains("text")) {
                    content += item["text"];
                }
            }
            
            // Log successful interaction
            get_logger().log(LogLevel::Info, std::format("Claude API request successful. Response length: {}", content.length()));
            
            return content;
            
        } catch (const std::exception& e) {
            get_logger().log(LogLevel::Error, std::format("Claude API error: {}", e.what()));
            return std::unexpected(ApiErrorInfo{ApiError::Unknown, std::format("Error: {}", e.what())});
        }
    });
}