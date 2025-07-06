#include "MCPToolManager.hpp"
#include "MCPClient.hpp"
#include "MCPProtocol.hpp"
#include "GlobalLogger.hpp"
#include <future>
#include <format>

MCPToolManager::MCPToolManager(MCPClient* client)
    : client_(client) {}

MCPToolManager::~MCPToolManager() = default;

std::vector<nlohmann::json> MCPToolManager::list_tools(std::optional<std::string> cursor) {
    if (!cursor.has_value() && !tool_cache_.empty()) {
        return tool_cache_;
    }
    auto request = MCPProtocolMessages::create_tools_list_request(cursor);
    auto fut = client_->send_request_for_manager(request);
    auto result = fut.get();
    if (!result || result->is_error() || !result->result.has_value()) {
        // Log the specific error for debugging
        if (!result) {
            get_logger().log(LogLevel::Error, "MCPToolManager::list_tools - No result from request");
        } else if (result->is_error()) {
            get_logger().log(LogLevel::Error, std::format("MCPToolManager::list_tools - Error response: {}", result->error->message));
        } else {
            get_logger().log(LogLevel::Error, "MCPToolManager::list_tools - No result field in response");
        }
        return {};
    }
    const auto& res = result->result.value();
    get_logger().log(LogLevel::Debug, std::format("MCPToolManager::list_tools - Response: {}", res.dump()));
    
    if (res.contains("tools") && res["tools"].is_array()) {
        tool_cache_ = res["tools"].get<std::vector<nlohmann::json>>();
        if (res.contains("cursor") && res["cursor"].is_string()) {
            last_cursor_ = res["cursor"].get<std::string>();
        }
        get_logger().log(LogLevel::Info, std::format("MCPToolManager::list_tools - Successfully loaded {} tools", tool_cache_.size()));
        return tool_cache_;
    } else {
        get_logger().log(LogLevel::Error, std::format("MCPToolManager::list_tools - Response does not contain tools array: {}", res.dump()));
    }
    return {};
}

nlohmann::json MCPToolManager::call_tool(const std::string& name, std::optional<nlohmann::json> arguments) {
    // Notify start of tool call
    if (notifier_) {
        notifier_->on_tool_call_start(name, arguments.value_or(nlohmann::json::object()));
        notifier_->on_mcp_activity(std::format("Calling tool: {}", name));
    }
    
    auto request = MCPProtocolMessages::create_tools_call_request(name, arguments);
    auto fut = client_->send_request_for_manager(request);
    auto result = fut.get();
    
    if (!result || result->is_error() || !result->result.has_value()) {
        // Notify error
        if (notifier_) {
            std::string error_msg = "Tool call failed";
            if (result && result->is_error()) {
                error_msg = result->error->message;
            }
            notifier_->on_tool_call_error(name, error_msg);
            notifier_->on_mcp_activity(std::format("Tool call failed: {}", error_msg));
        }
        return nlohmann::json();
    }
    
    // Notify success
    if (notifier_) {
        notifier_->on_tool_call_success(name, result->result.value());
        notifier_->on_mcp_activity(std::format("Tool call completed: {}", name));
    }
    
    return result->result.value();
}

bool MCPToolManager::validate_parameters(const std::string& name, const nlohmann::json& arguments) {
    // TODO: Implement parameter validation if tool schemas are available
    return true;
}

void MCPToolManager::handle_progress_notification(const nlohmann::json& progress) {
    // TODO: Implement progress tracking
}

void MCPToolManager::handle_list_changed_notification() {
    // Clear cache to force refresh on next list_tools() call
    clear_cache();
}

void MCPToolManager::set_notification_interface(MCPNotificationInterface* notifier) {
    notifier_ = notifier;
}

void MCPToolManager::clear_cache() {
    tool_cache_.clear();
    last_cursor_.clear();
} 