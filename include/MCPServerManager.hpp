#pragma once
#include "MCPServerConfig.hpp"
#include "MCPClient.hpp"
#include <memory>
#include <map>
#include <vector>
#include <expected>
#include <unistd.h> // For pid_t

// Structure to hold process information for stdio servers
struct MCPProcessInfo {
    pid_t pid;
    int stdin_fd;
    int stdout_fd;
};

class MCPServerManager {
public:
    MCPServerManager();
    ~MCPServerManager();
    
    // Initialize with configuration
    std::expected<void, MCPServerError> initialize(const std::string& config_path = "mcp_config.json");
    
    // Connect to all enabled servers
    std::expected<void, MCPServerError> connect_all();
    
    // Connect to a specific server
    std::expected<void, MCPServerError> connect_server(const std::string& name);
    
    // Disconnect from all servers
    void disconnect_all();
    
    // Disconnect from a specific server
    void disconnect_server(const std::string& name);
    
    // Get connected servers
    std::vector<std::string> get_connected_servers() const;
    
    // Get all available servers (from config)
    std::vector<std::string> get_available_servers() const;
    
    // Get server info
    std::expected<MCPServerConfiguration, MCPServerError> get_server_info(const std::string& name) const;
    
    // Get client for a specific server
    std::shared_ptr<MCPClient> get_client(const std::string& name) const;
    
    // Check if server is connected
    bool is_connected(const std::string& name) const;
    
    // Reload configuration
    std::expected<void, MCPServerError> reload_config();
    
    // Get configuration manager
    const MCPServerConfig& config() const { return config_; }
    MCPServerConfig& config() { return config_; }
    
    // Health check on all connected servers
    void health_check();
    
private:
    MCPServerConfig config_;
    std::map<std::string, std::shared_ptr<MCPClient>> clients_;
    std::map<std::string, bool> connection_status_;
    std::map<std::string, MCPProcessInfo> stdio_processes_;
    
    // Create MCP client for server
    std::expected<std::shared_ptr<MCPClient>, MCPServerError> create_client(const MCPServerConfiguration& server);
    
    // Start server process (for stdio-based servers)
    std::expected<void, MCPServerError> start_server_process(const MCPServerConfiguration& server);
    
    // Stop server process (for stdio-based servers)
    void stop_server_process(const std::string& name);
    
    // Log connection status
    void log_connection_status(const std::string& name, bool connected, const std::string& error = "");
};