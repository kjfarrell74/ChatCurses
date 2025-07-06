#include "MCPService.hpp"
#include "GlobalLogger.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <format>

void test_mcp_service_configuration() {
    std::cout << "=== Testing MCP Service Configuration ===" << std::endl;
    
    auto& mcp = MCPService::instance();
    
    // Test 1: Check initial state
    std::cout << "1. Initial state - configured: " << (mcp.is_configured() ? "Yes" : "No") << std::endl;
    std::cout << "   Initial state - connected: " << (mcp.is_connected() ? "Yes" : "No") << std::endl;
    
    // Test 2: Configure with YouTube transcript server
    std::cout << "\n2. Configuring MCP service for YouTube transcript server..." << std::endl;
    mcp.configure("ws://localhost:9091");
    std::cout << "   After configure - configured: " << (mcp.is_configured() ? "Yes" : "No") << std::endl;
    
    // Give some time for connection to establish
    std::cout << "   Waiting for connection to establish..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    std::cout << "   After wait - connected: " << (mcp.is_connected() ? "Yes" : "No") << std::endl;
    
    // Test 3: Try to trigger connection explicitly
    std::cout << "\n3. Testing explicit connection..." << std::endl;
    auto tools = mcp.list_available_tools();
    std::cout << "   Tools retrieved: " << tools.size() << std::endl;
    std::cout << "   After tools call - connected: " << (mcp.is_connected() ? "Yes" : "No") << std::endl;
    
    // Test 4: Check if we can get more details
    std::cout << "\n4. Debug connection details..." << std::endl;
    if (tools.empty()) {
        std::cout << "   No tools found - checking connection state..." << std::endl;
        // Let's try a longer wait
        std::this_thread::sleep_for(std::chrono::seconds(2));
        tools = mcp.list_available_tools();
        std::cout << "   After longer wait - tools: " << tools.size() << std::endl;
    }
    
    // Print any tool details we might have
    for (const auto& tool : tools) {
        std::cout << "   Tool JSON: " << tool.dump() << std::endl;
    }
}

void test_tool_detection() {
    std::cout << "\n=== Testing Tool Detection Logic ===" << std::endl;
    
    auto& mcp = MCPService::instance();
    
    std::vector<std::string> test_messages = {
        "Hello world",
        "Please scrape this website: https://example.com",
        "Can you get data from https://news.ycombinator.com?",
        "What's the weather today?",
        "Extract information from this page",
        "Download the content from the site",
        "Search for information about AI"
    };
    
    for (const auto& msg : test_messages) {
        bool should_use = mcp.should_use_tools(msg);
        std::cout << "Message: \"" << msg << "\" -> Should use tools: " << (should_use ? "Yes" : "No") << std::endl;
    }
}

void test_tool_listing() {
    std::cout << "\n=== Testing Tool Listing ===" << std::endl;
    
    auto& mcp = MCPService::instance();
    
    std::cout << "Attempting to list available tools..." << std::endl;
    auto tools = mcp.list_available_tools();
    
    std::cout << "Found " << tools.size() << " tools:" << std::endl;
    for (const auto& tool : tools) {
        std::cout << "  - ";
        if (tool.contains("name")) {
            std::cout << "Name: " << tool["name"].get<std::string>();
        }
        if (tool.contains("description")) {
            std::cout << ", Description: " << tool["description"].get<std::string>();
        }
        std::cout << std::endl;
    }
    
    // Test tools description
    std::cout << "\nTools description for AI context:" << std::endl;
    std::string desc = mcp.get_tools_description();
    if (!desc.empty()) {
        std::cout << desc << std::endl;
    } else {
        std::cout << "No tools description available" << std::endl;
    }
}

void test_tool_calling() {
    std::cout << "\n=== Testing Tool Calling ===" << std::endl;
    
    auto& mcp = MCPService::instance();
    
    // First list tools to see what's available
    auto tools = mcp.list_available_tools();
    if (tools.empty()) {
        std::cout << "No tools available for testing" << std::endl;
        return;
    }
    
    // Look for the get_transcript tool
    std::string transcript_tool_name;
    for (const auto& tool : tools) {
        if (tool.contains("name")) {
            std::string name = tool["name"].get<std::string>();
            if (name == "get_transcript") {
                transcript_tool_name = name;
                break;
            }
        }
    }
    
    if (transcript_tool_name.empty()) {
        std::cout << "No get_transcript tool found" << std::endl;
        return;
    }
    
    std::cout << "Testing tool: " << transcript_tool_name << std::endl;
    
    // Test with a YouTube URL
    nlohmann::json args = {{"url", "https://www.youtube.com/watch?v=n5DiIJQpI9o"}};
    
    std::cout << "Calling tool with YouTube URL: https://www.youtube.com/watch?v=n5DiIJQpI9o" << std::endl;
    auto result = mcp.call_tool(transcript_tool_name, args);
    
    if (result) {
        std::cout << "Tool call successful!" << std::endl;
        std::cout << "Result: " << result->dump(2) << std::endl;
    } else {
        std::cout << "Tool call failed" << std::endl;
    }
}

int main() {
    std::cout << "ChatCurses MCP Tools Test Program" << std::endl;
    std::cout << "=================================" << std::endl;
    
    // Initialize logger
    get_logger().set_level(LogLevel::Debug);
    
    try {
        test_mcp_service_configuration();
        test_tool_detection();
        test_tool_listing();
        test_tool_calling();
        
        std::cout << "\n=== Test Complete ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error during testing: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}