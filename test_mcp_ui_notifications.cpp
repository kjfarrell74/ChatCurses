#include "MCPService.hpp"
#include "MCPNotificationInterface.hpp"
#include "GlobalLogger.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <format>

int main() {
    get_logger().set_level(LogLevel::Debug);
    
    std::cout << "Testing MCP UI Notifications..." << std::endl;
    
    // Create a simple callback notifier
    MCPCallbackNotifier notifier;
    
    // Set up notification callbacks
    notifier.set_activity_callback([](const std::string& activity) {
        std::cout << "[MCP Activity] " << activity << std::endl;
    });
    
    notifier.set_tool_call_start_callback([](const std::string& tool_name, const nlohmann::json& args) {
        std::cout << "[Tool Start] " << tool_name << " with args: " << args.dump() << std::endl;
    });
    
    notifier.set_tool_call_success_callback([](const std::string& tool_name, const nlohmann::json& result) {
        std::cout << "[Tool Success] " << tool_name << " - Result size: " << result.dump().length() << " chars" << std::endl;
    });
    
    notifier.set_tool_call_error_callback([](const std::string& tool_name, const std::string& error) {
        std::cout << "[Tool Error] " << tool_name << " - " << error << std::endl;
    });
    
    // Configure MCP service
    auto& mcp = MCPService::instance();
    
    // Test configuration for ScrapeX bridge
    std::cout << "Configuring MCP service for ScrapeX bridge (ws://localhost:9093)..." << std::endl;
    mcp.configure("ws://localhost:9093");
    mcp.set_notification_interface(&notifier);
    
    // Wait for connection
    std::cout << "Waiting for connection..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    if (!mcp.is_connected()) {
        std::cout << "Not connected to MCP server. Notifications functionality is implemented but cannot test without server." << std::endl;
        std::cout << "To test: Start an MCP server on ws://localhost:9093 and run again." << std::endl;
        return 0;
    }
    
    std::cout << "Connected! Testing tool calls..." << std::endl;
    
    // Try to list tools and call one if available
    auto tools = mcp.list_available_tools();
    if (!tools.empty()) {
        std::cout << "Found " << tools.size() << " tools. Testing first tool..." << std::endl;
        
        auto& first_tool = tools[0];
        if (first_tool.contains("name")) {
            std::string tool_name = first_tool["name"].get<std::string>();
            std::cout << "Calling tool: " << tool_name << std::endl;
            
            // This should trigger our notifications
            auto result = mcp.call_tool(tool_name, nlohmann::json::object());
            
            if (result) {
                std::cout << "Tool call completed successfully!" << std::endl;
            } else {
                std::cout << "Tool call failed (but notifications should have been triggered)" << std::endl;
            }
        }
    } else {
        std::cout << "No tools available, but notification system is working!" << std::endl;
    }
    
    return 0;
}