#include "MCPServerConfig.hpp"
#include "MCPToolService.hpp"
#include <iostream>

int main() {
    std::cout << "=== MCP Configuration Test ===" << std::endl;
    
    // Test 1: Load config
    MCPServerConfig config("mcp_config.json");
    auto load_result = config.load();
    if (load_result.has_value()) {
        std::cout << "✓ Config loaded successfully" << std::endl;
        auto servers = config.servers();
        std::cout << "Found " << servers.size() << " servers:" << std::endl;
        for (const auto& [name, server] : servers) {
            std::cout << "  - " << name << " (enabled: " << (server.enabled ? "yes" : "no") << ")" << std::endl;
        }
    } else {
        std::cout << "✗ Failed to load config" << std::endl;
    }
    
    // Test 2: Test tool detection
    std::cout << "\n=== Tool Detection Test ===" << std::endl;
    std::vector<std::string> test_messages = {
        "List files in directory",
        "Search for cats",
        "What is the weather?",
        "**TOOL_CALL: list_files {}**"
    };
    
    for (const auto& msg : test_messages) {
        bool should_use = MCPToolService::instance().should_process_with_tools(msg);
        std::cout << "\"" << msg << "\" -> " << (should_use ? "use tools" : "no tools") << std::endl;
    }
    
    std::cout << "\n=== Test Complete ===" << std::endl;
    return 0;
}