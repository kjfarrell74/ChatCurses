#pragma once
#include "AIClientInterface.hpp"
#include "MCPService.hpp"
#include <regex>

class BaseAIClient : public AIClientInterface {
public:
    void set_api_key(const std::string& key) override {
        std::lock_guard lock(mutex_);
        api_key_ = key;
    }
    
    void set_system_prompt(const std::string& prompt) override {
        std::lock_guard lock(mutex_);
        system_prompt_ = prompt;
    }
    
    void set_model(const std::string& model) override {
        std::lock_guard lock(mutex_);
        model_ = model;
    }
    
    void clear_history() override {
        std::lock_guard lock(mutex_);
        conversation_history_.clear();
    }
    
    void push_user_message(const std::string& content) override {
        std::lock_guard lock(mutex_);
        conversation_history_.push_back({{"role", "user"}, {"content", content}});
    }
    
    void push_assistant_message(const std::string& content) override {
        std::lock_guard lock(mutex_);
        conversation_history_.push_back({{"role", "assistant"}, {"content", content}});
    }

protected:
    std::string api_key_;
    std::string system_prompt_;
    std::string model_;
    mutable std::mutex mutex_;
    std::vector<nlohmann::json> conversation_history_;

    // MCP integration helpers
    std::string enhance_system_prompt_with_tools(const std::string& base_prompt) const {
        auto& mcp = MCPService::instance();
        if (!mcp.is_configured() || !mcp.is_connected()) {
            return base_prompt;
        }
        
        std::string enhanced = base_prompt;
        if (!enhanced.empty()) {
            enhanced += "\n\n";
        }
        
        enhanced += "You have access to the following tools that you can use to help answer questions:";
        enhanced += mcp.get_tools_description();
        enhanced += "\nWhen a user asks for something that could benefit from these tools, use them appropriately.";
        
        return enhanced;
    }

    std::string process_with_mcp_tools(const std::string& user_message) const {
        try {
            auto& mcp = MCPService::instance();
            
            // Check if this message might benefit from tools
            if (!mcp.is_configured() || !mcp.is_connected() || !mcp.should_use_tools(user_message)) {
                return ""; // No tool usage needed
            }
        
        std::string tool_results = "";
        
        // Check for URLs in the message
        std::regex url_pattern(R"(https?://[^\s]+)");
        std::smatch url_match;
        if (std::regex_search(user_message, url_match, url_pattern)) {
            std::string url = url_match.str();
            
            // Check if it's a YouTube URL
            std::regex youtube_pattern(R"((?:https?://)?(?:www\.)?(?:youtube\.com/watch\?v=|youtu\.be/))");
            if (std::regex_search(url, youtube_pattern)) {
                // Try to use get_transcript tool for YouTube URLs
                nlohmann::json transcript_args = {{"url", url}};
                auto result = mcp.call_tool("get_transcript", transcript_args);
                if (result) {
                    tool_results += "YouTube transcript from " + url + ":\n";
                    if (result->contains("content") && result->at("content").is_array()) {
                        auto content_array = result->at("content");
                        for (const auto& item : content_array) {
                            if (item.contains("text")) {
                                tool_results += item.at("text").get<std::string>() + "\n";
                            }
                        }
                    } else if (result->contains("content")) {
                        tool_results += result->at("content").get<std::string>() + "\n";
                    } else {
                        tool_results += result->dump() + "\n";
                    }
                    tool_results += "\n";
                }
            } else {
                // Try to use scraping tool for other URLs
                nlohmann::json scrape_args = {{"url", url}};
                auto result = mcp.call_tool("scrape_url", scrape_args);
                if (result) {
                    tool_results += "Scraped content from " + url + ":\n";
                    if (result->contains("content")) {
                        tool_results += result->at("content").get<std::string>() + "\n\n";
                    } else {
                        tool_results += result->dump() + "\n\n";
                    }
                }
            }
        }
        
        // Add more tool patterns here as needed
        
        return tool_results;
        } catch (const std::exception& e) {
            // If MCP fails, don't break the whole request
            return "";
        }
    }
};