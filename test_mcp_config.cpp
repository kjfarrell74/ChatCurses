#include "MCPServerConfig.hpp"
#include "MCPServerManager.hpp"
#include "GlobalLogger.hpp"
#include <iostream>
#include <filesystem>

int main() {
    get_logger().log(LogLevel::Info, "Starting MCP server configuration test");
    
    // Test 1: Create and save default configuration
    {
        MCPServerConfig config("test_mcp_config.json");
        config.create_default_config();
        
        auto save_result = config.save();
        if (save_result.has_value()) {
            get_logger().log(LogLevel::Info, "✓ Default configuration saved successfully");
        } else {
            get_logger().log(LogLevel::Error, "✗ Failed to save default configuration");
            return 1;
        }
    }
    
    // Test 2: Load configuration from file
    {
        MCPServerConfig config("test_mcp_config.json");
        auto load_result = config.load();
        if (load_result.has_value()) {
            get_logger().log(LogLevel::Info, "✓ Configuration loaded successfully");
            
            // List all servers
            const auto& servers = config.servers();
            get_logger().log(LogLevel::Info, std::format("Found {} servers in configuration:", servers.size()));
            for (const auto& [name, server] : servers) {
                get_logger().log(LogLevel::Info, std::format("  - {} ({}): {}", name, server.enabled ? "enabled" : "disabled", server.description));
            }
        } else {
            get_logger().log(LogLevel::Error, "✗ Failed to load configuration");
            return 1;
        }
    }
    
    // Test 3: Test MCPServerManager initialization
    {
        MCPServerManager manager;
        auto init_result = manager.initialize("test_mcp_config.json");
        if (init_result.has_value()) {
            get_logger().log(LogLevel::Info, "✓ MCPServerManager initialized successfully");
            
            // Get available servers
            auto available_servers = manager.get_available_servers();
            get_logger().log(LogLevel::Info, "Available servers:");
            for (const auto& server : available_servers) {
                get_logger().log(LogLevel::Info, std::format("  - {}", server));
            }
            
            // Get enabled servers
            auto enabled_servers = manager.config().get_enabled_servers();
            get_logger().log(LogLevel::Info, "Enabled servers:");
            for (const auto& server : enabled_servers) {
                get_logger().log(LogLevel::Info, std::format("  - {}", server));
            }
            
        } else {
            get_logger().log(LogLevel::Error, "✗ Failed to initialize MCPServerManager");
            return 1;
        }
    }
    
    // Test 4: Test server configuration modification
    {
        MCPServerConfig config("test_mcp_config.json");
        config.load();
        
        // Add a custom server
        MCPServerConfiguration custom_server;
        custom_server.name = "custom-test-server";
        custom_server.command = "echo";
        custom_server.args = {"hello", "world"};
        custom_server.description = "Test server for debugging";
        custom_server.enabled = true;
        custom_server.connection_type = "stdio";
        
        config.add_server("custom-test-server", custom_server);
        
        auto save_result = config.save();
        if (save_result.has_value()) {
            get_logger().log(LogLevel::Info, "✓ Custom server added and saved successfully");
        } else {
            get_logger().log(LogLevel::Error, "✗ Failed to save custom server configuration");
            return 1;
        }
    }
    
    // Cleanup
    if (std::filesystem::exists("test_mcp_config.json")) {
        std::filesystem::remove("test_mcp_config.json");
        get_logger().log(LogLevel::Info, "✓ Test configuration file cleaned up");
    }
    
    get_logger().log(LogLevel::Info, "All MCP server configuration tests passed!");
    return 0;
}