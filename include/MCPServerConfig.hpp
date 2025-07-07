#pragma once
#include <string>
#include <map>
#include <vector>
#include <expected>
#include <nlohmann/json.hpp>

enum class MCPServerError {
    ConfigNotFound,
    ConfigParseError,
    ServerNotFound,
    ConnectionError,
    InitializationError,
    Unknown,
    ProcessSpawnError
};

struct MCPServerConfiguration {
    std::string name;
    std::string command;
    std::vector<std::string> args;
    std::map<std::string, std::string> env;
    std::string description;
    bool enabled = true;
    
    // For servers that use direct connections (WebSocket/HTTP)
    std::string url;
    std::string connection_type = "stdio"; // "stdio", "websocket", "http"
    
    // Convert to JSON for persistence
    nlohmann::json to_json() const;
    
    // Create from JSON
    static std::expected<MCPServerConfiguration, MCPServerError> from_json(const nlohmann::json& j);
};

class MCPServerConfig {
public:
    MCPServerConfig(const std::string& config_path = "mcp_config.json");
    
    // Load MCP server configuration from file
    std::expected<void, MCPServerError> load();
    
    // Save MCP server configuration to file
    std::expected<void, MCPServerError> save() const;
    
    // Get all configured servers
    const std::map<std::string, MCPServerConfiguration>& servers() const { return servers_; }
    
    // Get a specific server by name
    std::expected<MCPServerConfiguration, MCPServerError> get_server(const std::string& name) const;
    
    // Add or update a server configuration
    void add_server(const std::string& name, const MCPServerConfiguration& server);
    
    // Remove a server configuration
    bool remove_server(const std::string& name);
    
    // Get enabled servers only
    std::vector<std::string> get_enabled_servers() const;
    
    // Create default configuration with common MCP servers
    void create_default_config();
    
    const std::string& config_path() const { return config_path_; }
    
private:
    std::string config_path_;
    std::map<std::string, MCPServerConfiguration> servers_;
    
    // Convert error to string for logging
    std::string error_to_string(MCPServerError error) const;
};