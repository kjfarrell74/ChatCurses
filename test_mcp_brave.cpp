#include "MCPService.hpp"
#include "GlobalLogger.hpp"
#include <iostream>
#include <format>

int main() {
    // Initialize logger
    auto& logger = get_logger();
    logger.log(LogLevel::Info, "Testing MCP Brave Search integration");
    
    // Configure MCPService to use Brave search
    auto& mcp = MCPService::instance();
    mcp.configure("ws://localhost:9092");
    
    std::cout << "MCP Service configured for Brave search on port 9092" << std::endl;
    
    // Test if it should use tools for search queries
    std::string test_queries[] = {
        "search for information about AI",
        "find recent news about ChatGPT",
        "what is the weather today",
        "hello world"
    };
    
    for (const auto& query : test_queries) {
        bool should_use = mcp.should_use_tools(query);
        std::cout << std::format("Query: '{}' -> Should use tools: {}", query, should_use ? "YES" : "NO") << std::endl;
    }
    
    // Test listing available tools
    std::cout << "\nTesting tool discovery..." << std::endl;
    auto tools = mcp.list_available_tools();
    std::cout << std::format("Found {} tools", tools.size()) << std::endl;
    
    for (const auto& tool : tools) {
        if (tool.contains("name")) {
            std::cout << "- " << tool["name"].get<std::string>();
            if (tool.contains("description")) {
                std::cout << ": " << tool["description"].get<std::string>();
            }
            std::cout << std::endl;
        }
    }
    
    // Test tool calling
    if (!tools.empty()) {
        std::cout << "\nTesting tool calling..." << std::endl;
        
        // Try to call the first available tool with a test query
        auto first_tool = tools[0];
        if (first_tool.contains("name")) {
            std::string tool_name = first_tool["name"];
            std::cout << std::format("Calling tool: {}", tool_name) << std::endl;
            
            nlohmann::json args = {{"query", "ChatGPT news"}};
            auto result = mcp.call_tool(tool_name, args);
            
            if (result) {
                std::cout << "Tool call successful!" << std::endl;
                std::cout << "Result: " << result->dump(2) << std::endl;
            } else {
                std::cout << "Tool call failed" << std::endl;
            }
        }
    }
    
    return 0;
}