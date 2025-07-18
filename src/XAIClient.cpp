#include "XAIClient.hpp"
#include "GlobalLogger.hpp"
#include "MCPService.hpp"
#include "MCPToolService.hpp"
#include <format>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <regex>

std::string XAIClient::enhance_system_prompt_with_tools(const std::string& original_prompt) {
    auto& tool_service = MCPToolService::instance();
    
    std::string tools_description = tool_service.get_tools_description_for_ai();
    if (tools_description.empty()) {
        return original_prompt;
    }

    return original_prompt + tools_description;
}

std::string XAIClient::process_with_mcp_tools(const std::string& user_message) {
    auto& tool_service = MCPToolService::instance();
    
    // Try auto tool calling based on message content
    auto result = tool_service.auto_call_tools(user_message);
    if (result.has_value()) {
        get_logger().log(LogLevel::Info, "MCP tool called automatically, returning results");
        return result->dump(2);
    }

    return "";
}

std::string XAIClient::process_tool_calls_in_response(const std::string& ai_response) {
    auto& tool_service = MCPToolService::instance();
    
    // Detect tool calls in the AI response using the format: **TOOL_CALL: tool_name {"param": "value"}**
    auto tool_calls = tool_service.detect_tool_calls_in_message(ai_response);
    
    if (tool_calls.empty()) {
        return ai_response; // No tool calls to process
    }
    
    std::string processed_response = ai_response;
    
    // Process each tool call
    for (const auto& tool_call_str : tool_calls) {
        get_logger().log(LogLevel::Info, std::format("Processing tool call: {}", tool_call_str));
        
        // Parse the tool call format: **TOOL_CALL: tool_name {...}**
        std::regex tool_call_regex(R"(\*\*TOOL_CALL:\s*(\w+)\s*(\{[^}]*\})\*\*)");
        std::smatch match;
        
        if (std::regex_search(tool_call_str, match, tool_call_regex)) {
            std::string tool_name = match[1].str();
            std::string args_str = match[2].str();
            
            try {
                nlohmann::json args = nlohmann::json::parse(args_str);
                
                // Call the tool
                auto result = tool_service.call_tool(tool_name, args);
                
                if (result.has_value()) {
                    // Format the result nicely
                    std::string tool_result = result->dump(2);
                    std::string formatted_result = std::format("\n\n**Tool Result ({})**:\n```json\n{}\n```\n\n", tool_name, tool_result);
                    
                    // Replace the tool call with the result
                    processed_response = std::regex_replace(processed_response, 
                        std::regex(std::regex_replace(tool_call_str, std::regex(R"([\[\]{}()*+?.^$|\\])"), R"(\$&)")), 
                        formatted_result);
                        
                    get_logger().log(LogLevel::Info, std::format("Tool '{}' executed successfully", tool_name));
                } else {
                    // Replace with error message
                    std::string error_result = std::format("\n\n**Tool Error ({})**: Tool execution failed\n\n", tool_name);
                    processed_response = std::regex_replace(processed_response, 
                        std::regex(std::regex_replace(tool_call_str, std::regex(R"([\[\]{}()*+?.^$|\\])"), R"(\$&)")), 
                        error_result);
                        
                    get_logger().log(LogLevel::Warning, std::format("Tool '{}' execution failed", tool_name));
                }
                
            } catch (const std::exception& e) {
                get_logger().log(LogLevel::Error, std::format("Error parsing tool call arguments: {}", e.what()));
                
                // Replace with error message
                std::string error_result = std::format("\n\n**Tool Error ({})**: Invalid arguments\n\n", tool_name);
                processed_response = std::regex_replace(processed_response, 
                    std::regex(std::regex_replace(tool_call_str, std::regex(R"([\[\]{}()*+?.^$|\\])"), R"(\$&)")), 
                    error_result);
            }
        }
    }
    
    return processed_response;
}

size_t XAIWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    static_cast<std::string*>(userp)->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

std::future<std::expected<std::string, ApiErrorInfo>> XAIClient::send_message(
    const nlohmann::json& messages, 
    const std::string& model) {
    
    return std::async(std::launch::async, [this, messages, model]() -> std::expected<std::string, ApiErrorInfo> {
        std::lock_guard lock(mutex_);
        
        if (api_key_.empty()) {
            return std::unexpected(ApiErrorInfo{ApiError::ApiKeyNotSet, "API key not set"});
        }
        
        try {
            // Enhance system prompt with tools
            std::string enhanced_prompt = enhance_system_prompt_with_tools(system_prompt_);
            
            // Build request body
            nlohmann::json request_body;
            request_body["model"] = model.empty() ? model_ : model;
            request_body["temperature"] = 0.7;
            request_body["max_tokens"] = 4000;
            
            // Build messages with enhanced system prompt
            nlohmann::json modified_messages = nlohmann::json::array();
            
            // Add enhanced system message if present
            if (!enhanced_prompt.empty()) {
                modified_messages.push_back({
                    {"role", "system"},
                    {"content", enhanced_prompt}
                });
            }
            
            // Add original messages (excluding system messages since we handled them above)
            for (const auto& msg : messages) {
                if (msg.contains("role") && msg["role"] != "system") {
                    modified_messages.push_back(msg);
                }
            }

            // Process with MCP tools if needed - check the last user message
            if (!messages.empty()) {
                auto last_message = messages.back();
                if (last_message.contains("content")) {
                    std::string user_message = last_message["content"];
                    std::string tool_results = process_with_mcp_tools(user_message);
                    if (!tool_results.empty()) {
                        get_logger().log(LogLevel::Info, std::format("[MCP TOOL] Tool results injected: {}", tool_results));
                        modified_messages.push_back({
                            {"role", "system"},
                            {"content", "_[TOOL] " + tool_results + "_"}
                        });
                    }
                }
            }
            
            request_body["messages"] = modified_messages;
            
            // Debug: Log the request being sent
            get_logger().log(LogLevel::Debug, std::format("XAI Request JSON: {}", request_body.dump()));
            
            // Prepare HTTP request
            CURL* curl = curl_easy_init();
            if (!curl) {
                return std::unexpected(ApiErrorInfo{ApiError::CurlInitFailed, "Failed to initialize curl"});
            }
            
            std::string response_string;
            struct curl_slist* headers = nullptr;
            
            // Set headers
            std::string auth_header = "Authorization: Bearer " + api_key_;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            headers = curl_slist_append(headers, auth_header.c_str());
            
            // Set curl options
            std::string json_data = request_body.dump();
            curl_easy_setopt(curl, CURLOPT_URL, "https://api.x.ai/v1/chat/completions");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, json_data.length());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, XAIWriteCallback);
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
            
            if (!response_json.contains("choices") || response_json["choices"].empty()) {
                return std::unexpected(ApiErrorInfo{ApiError::MalformedResponse, "Invalid response format"});
            }
            
            auto choice = response_json["choices"][0];
            if (!choice.contains("message") || !choice["message"].contains("content")) {
                return std::unexpected(ApiErrorInfo{ApiError::MalformedResponse, "Invalid response format"});
            }
            
            std::string content = choice["message"]["content"];
            
            // Process any tool calls in the AI response
            std::string final_content = process_tool_calls_in_response(content);
            
            // Log successful interaction
            get_logger().log(LogLevel::Info, std::format("XAI API request successful. Response length: {}", final_content.length()));
            
            return final_content;
            
        } catch (const std::exception& e) {
            get_logger().log(LogLevel::Error, std::format("XAI API error: {}", e.what()));
            return std::unexpected(ApiErrorInfo{ApiError::Unknown, std::format("Error: {}", e.what())});
        }
    });
}

std::future<std::expected<std::string, ApiErrorInfo>> XAIClient::send_message(
    const std::string& prompt,
    const std::string& model) {
    auto messages = build_message_history(prompt);
    return send_message(messages, model);
}

void XAIClient::send_message_stream(
    const std::string& prompt,
    const std::string& model,
    std::function<void(const std::string& chunk, bool is_last_chunk)> on_chunk_cb,
    std::function<void()> on_done_cb,
    std::function<void(const ApiErrorInfo& error)> on_error_cb) {
    
    // For now, use the non-streaming version
    std::thread([this, prompt, model, on_chunk_cb, on_done_cb, on_error_cb]() {
        auto messages = this->build_message_history(prompt);
        auto fut = this->send_message(messages, model);
        auto result = fut.get();
        if (result) {
            on_chunk_cb(*result, true);
            on_done_cb();
        } else {
            on_error_cb(result.error());
        }
    }).detach();
}

std::vector<std::string> XAIClient::available_models() const {
    return {"grok-beta", "grok-2", "grok-3-beta"};
}