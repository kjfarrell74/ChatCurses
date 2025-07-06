#pragma once
#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
#include "MCPNotificationInterface.hpp"

class MCPClient;

class MCPToolManager {
public:
    explicit MCPToolManager(MCPClient* client);
    ~MCPToolManager();

    // List tools (optionally paginated)
    std::vector<nlohmann::json> list_tools(std::optional<std::string> cursor = std::nullopt);

    // Call a tool by name with arguments
    nlohmann::json call_tool(const std::string& name, std::optional<nlohmann::json> arguments = std::nullopt);

    // Validate tool parameters
    bool validate_parameters(const std::string& name, const nlohmann::json& arguments);

    // Progress tracking (stub for now)
    void handle_progress_notification(const nlohmann::json& progress);
    
    // Notification handler for tool list updates
    void handle_list_changed_notification();
    
    // Set notification interface for tool call events
    void set_notification_interface(MCPNotificationInterface* notifier);

private:
    MCPClient* client_;
    std::vector<nlohmann::json> tool_cache_;
    std::string last_cursor_;
    MCPNotificationInterface* notifier_ = nullptr;
    // Add more as needed
    void clear_cache();
}; 