#pragma once
#include "MCPClient.hpp"
#include "MCPNotificationInterface.hpp"
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

// Global MCP service that all AI providers can use
class MCPService {
public:
    static MCPService& instance() {
        static MCPService inst;
        return inst;
    }

    // Configuration
    void configure(const std::string& server_url);
    bool is_configured() const { return mcp_client_ != nullptr; }
    bool is_connected() const;

    // Tool operations
    std::vector<nlohmann::json> list_available_tools();
    std::optional<nlohmann::json> call_tool(const std::string& name, const nlohmann::json& arguments);

    // Resource operations  
    std::vector<nlohmann::json> list_available_resources();
    std::optional<nlohmann::json> read_resource(const std::string& uri);

    // Prompt operations
    std::vector<nlohmann::json> list_available_prompts();
    std::optional<std::string> get_prompt(const std::string& name, const nlohmann::json& arguments);

    // Check if a request might need MCP tools
    bool should_use_tools(const std::string& user_message);

    // Get tool descriptions for AI context
    std::string get_tools_description();
    
    // Set notification interface for tool call events
    void set_notification_interface(MCPNotificationInterface* notifier);

private:
    MCPService() = default;
    ~MCPService() = default;
    MCPService(const MCPService&) = delete;
    MCPService& operator=(const MCPService&) = delete;

    std::unique_ptr<MCPClient> mcp_client_;
    std::string current_server_url_;
    
    // Cache for tool/resource listings
    mutable std::vector<nlohmann::json> tools_cache_;
    mutable std::vector<nlohmann::json> resources_cache_;
    mutable std::vector<nlohmann::json> prompts_cache_;
    mutable bool cache_valid_ = false;
    
    void refresh_cache();
    void launch_bridge_if_needed();
};