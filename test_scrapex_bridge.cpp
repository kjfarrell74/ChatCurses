#include "MCPService.hpp"
#include "GlobalLogger.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    std::cout << "ChatCurses ScrapeX Bridge Test\n";
    std::cout << "==============================\n\n";
    
    // Test 1: Configure MCP service for scrapex
    std::cout << "1. Configuring MCP service for ScrapeX bridge...\n";
    MCPService::instance().configure("ws://localhost:9093");
    std::cout << "   Configured: " << (MCPService::instance().is_connected() ? "Yes" : "No") << "\n\n";
    
    // Test 2: Wait for connection
    std::cout << "2. Waiting for connection to establish...\n";
    std::this_thread::sleep_for(std::chrono::seconds(3));
    std::cout << "   Connected: " << (MCPService::instance().is_connected() ? "Yes" : "No") << "\n\n";
    
    // Test 3: List available tools
    std::cout << "3. Testing tool availability...\n";
    auto tools = MCPService::instance().list_available_tools();
    std::cout << "   Tools found: " << tools.size() << "\n";
    
    for (const auto& tool : tools) {
        if (tool.contains("name")) {
            std::cout << "   - " << tool["name"].get<std::string>();
            if (tool.contains("description")) {
                std::cout << ": " << tool["description"].get<std::string>();
            }
            std::cout << "\n";
        }
    }
    
    // Test 4: Test tool call if available
    if (!tools.empty()) {
        std::cout << "\n4. Testing scrape_tweet tool...\n";
        nlohmann::json args;
        args["url"] = "https://x.com/chatgpt21/status/1941530208676581473";
        
        auto result = MCPService::instance().call_tool("scrape_tweet", args);
        if (result) {
            std::cout << "   Tool call successful\n";
            std::cout << "   Result: " << result->dump(2) << "\n";
        } else {
            std::cout << "   Tool call failed\n";
        }
    }
    
    std::cout << "\nTest completed.\n";
    return 0;
}