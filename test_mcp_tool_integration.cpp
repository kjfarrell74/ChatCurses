#include "MCPServerManager.hpp"
#include "MCPToolService.hpp"
#include "GlobalLogger.hpp"
#include <iostream>

int main() {
    get_logger().log(LogLevel::Info, "Starting MCP tool integration test");
    
    // Test 1: Initialize MCP server manager
    MCPServerManager manager;
    auto init_result = manager.initialize("mcp_config.json");
    if (!init_result.has_value()) {
        get_logger().log(LogLevel::Error, "Failed to initialize MCP server manager");
        return 1;
    }
    
    get_logger().log(LogLevel::Info, "✓ MCP server manager initialized");
    
    // Test 2: Connect to servers (even if they fail, we can test the service)
    auto connect_result = manager.connect_all();
    auto connected_servers = manager.get_connected_servers();
    get_logger().log(LogLevel::Info, std::format("Connected to {} servers", connected_servers.size()));
    
    // Test 3: Initialize tool service
    MCPToolService::instance().initialize(&manager);
    get_logger().log(LogLevel::Info, "✓ MCP tool service initialized");
    
    // Test 4: Discover tools
    auto tools = MCPToolService::instance().get_all_available_tools();
    get_logger().log(LogLevel::Info, std::format("Discovered {} tools", tools.size()));
    
    for (const auto& tool : tools) {
        get_logger().log(LogLevel::Info, std::format("  - {} (from {}): {}", tool.name, tool.server_name, tool.description));
    }
    
    // Test 5: Test tool descriptions for AI
    std::string ai_description = MCPToolService::instance().get_tools_description_for_ai();
    if (!ai_description.empty()) {
        get_logger().log(LogLevel::Info, "✓ AI tool descriptions generated");
        get_logger().log(LogLevel::Debug, std::format("Tool description preview: {}", ai_description.substr(0, 200)));
    } else {
        get_logger().log(LogLevel::Info, "No tools available for AI description");
    }
    
    // Test 6: Test tool detection
    std::vector<std::string> test_messages = {
        "Can you search for information about cats?",
        "List the files in my directory",
        "What is the weather like?",
        "**TOOL_CALL: search {\"query\": \"test\"}}**"
    };
    
    for (const auto& message : test_messages) {
        bool should_use_tools = MCPToolService::instance().should_process_with_tools(message);
        auto detected_calls = MCPToolService::instance().detect_tool_calls_in_message(message);
        
        get_logger().log(LogLevel::Info, std::format("Message: \"{}\"", message));
        get_logger().log(LogLevel::Info, std::format("  Should use tools: {}", should_use_tools ? "yes" : "no"));
        get_logger().log(LogLevel::Info, std::format("  Detected tool calls: {}", detected_calls.size()));
    }
    
    get_logger().log(LogLevel::Info, "✓ All MCP tool integration tests completed successfully!");
    return 0;
}