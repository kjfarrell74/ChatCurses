#pragma once
#include <string>
#include <functional>
#include <nlohmann/json.hpp>

// Interface for receiving MCP tool call notifications
class MCPNotificationInterface {
public:
    virtual ~MCPNotificationInterface() = default;
    
    // Called when a tool is about to be invoked
    virtual void on_tool_call_start(const std::string& tool_name, const nlohmann::json& arguments) = 0;
    
    // Called when a tool call completes successfully
    virtual void on_tool_call_success(const std::string& tool_name, const nlohmann::json& result) = 0;
    
    // Called when a tool call fails
    virtual void on_tool_call_error(const std::string& tool_name, const std::string& error_message) = 0;
    
    // Called when any MCP operation occurs (general notification)
    virtual void on_mcp_activity(const std::string& activity_description) = 0;
};

// Callback-based notification handler
class MCPCallbackNotifier : public MCPNotificationInterface {
public:
    using ToolCallStartCallback = std::function<void(const std::string&, const nlohmann::json&)>;
    using ToolCallSuccessCallback = std::function<void(const std::string&, const nlohmann::json&)>;
    using ToolCallErrorCallback = std::function<void(const std::string&, const std::string&)>;
    using ActivityCallback = std::function<void(const std::string&)>;
    
    void set_tool_call_start_callback(ToolCallStartCallback callback) {
        tool_call_start_callback_ = std::move(callback);
    }
    
    void set_tool_call_success_callback(ToolCallSuccessCallback callback) {
        tool_call_success_callback_ = std::move(callback);
    }
    
    void set_tool_call_error_callback(ToolCallErrorCallback callback) {
        tool_call_error_callback_ = std::move(callback);
    }
    
    void set_activity_callback(ActivityCallback callback) {
        activity_callback_ = std::move(callback);
    }

    void on_tool_call_start(const std::string& tool_name, const nlohmann::json& arguments) override {
        if (tool_call_start_callback_) {
            tool_call_start_callback_(tool_name, arguments);
        }
    }
    
    void on_tool_call_success(const std::string& tool_name, const nlohmann::json& result) override {
        if (tool_call_success_callback_) {
            tool_call_success_callback_(tool_name, result);
        }
    }
    
    void on_tool_call_error(const std::string& tool_name, const std::string& error_message) override {
        if (tool_call_error_callback_) {
            tool_call_error_callback_(tool_name, error_message);
        }
    }
    
    void on_mcp_activity(const std::string& activity_description) override {
        if (activity_callback_) {
            activity_callback_(activity_description);
        }
    }

private:
    ToolCallStartCallback tool_call_start_callback_;
    ToolCallSuccessCallback tool_call_success_callback_;
    ToolCallErrorCallback tool_call_error_callback_;
    ActivityCallback activity_callback_;
};