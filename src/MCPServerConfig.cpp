#include "MCPServerConfig.hpp"
#include "GlobalLogger.hpp"
#include <fstream>
#include <filesystem>

using json = nlohmann::json;

nlohmann::json MCPServerConfiguration::to_json() const {
    json j;
    j["name"] = name;
    j["command"] = command;
    j["args"] = args;
    j["env"] = env;
    j["description"] = description;
    j["enabled"] = enabled;
    j["url"] = url;
    j["connection_type"] = connection_type;
    return j;
}

std::expected<MCPServerConfiguration, MCPServerError> MCPServerConfiguration::from_json(const nlohmann::json& j) {
    try {
        MCPServerConfiguration info;
        info.name = j.value("name", "");
        info.command = j.value("command", "");
        info.args = j.value("args", std::vector<std::string>{});
        info.env = j.value("env", std::map<std::string, std::string>{});
        info.description = j.value("description", "");
        info.enabled = j.value("enabled", true);
        info.url = j.value("url", "");
        info.connection_type = j.value("connection_type", "stdio");
        
        return info;
    } catch (const std::exception& e) {
        get_logger().log(LogLevel::Error, std::format("Failed to parse MCP server info from JSON: {}", e.what()));
        return std::unexpected(MCPServerError::ConfigParseError);
    }
}

MCPServerConfig::MCPServerConfig(const std::string& config_path) : config_path_(config_path) {
    get_logger().log(LogLevel::Info, std::format("MCPServerConfig initialized with path: {}", config_path_));
}

std::expected<void, MCPServerError> MCPServerConfig::load() {
    try {
        if (!std::filesystem::exists(config_path_)) {
            get_logger().log(LogLevel::Info, std::format("MCP config file not found, creating default: {}", config_path_));
            create_default_config();
            return save();
        }
        
        std::ifstream file(config_path_);
        if (!file.is_open()) {
            get_logger().log(LogLevel::Error, std::format("Failed to open MCP config file: {}", config_path_));
            return std::unexpected(MCPServerError::ConfigNotFound);
        }
        
        json j;
        file >> j;
        
        servers_.clear();
        
        if (j.contains("mcpServers")) {
            for (const auto& [name, server_json] : j["mcpServers"].items()) {
                auto server_result = MCPServerConfiguration::from_json(server_json);
                if (server_result.has_value()) {
                    auto server = server_result.value();
                    server.name = name; // Ensure name matches key
                    servers_[name] = server;
                    get_logger().log(LogLevel::Debug, std::format("Loaded MCP server: {} ({})", name, server.description));
                } else {
                    get_logger().log(LogLevel::Warning, std::format("Failed to load MCP server config for: {}", name));
                }
            }
        }
        
        get_logger().log(LogLevel::Info, std::format("Loaded {} MCP servers from config", servers_.size()));
        return {};
        
    } catch (const std::exception& e) {
        get_logger().log(LogLevel::Error, std::format("Failed to load MCP config: {}", e.what()));
        return std::unexpected(MCPServerError::ConfigParseError);
    }
}

std::expected<void, MCPServerError> MCPServerConfig::save() const {
    try {
        json j;
        json servers_json;
        
        for (const auto& [name, server] : servers_) {
            servers_json[name] = server.to_json();
        }
        
        j["mcpServers"] = servers_json;
        
        std::ofstream file(config_path_);
        if (!file.is_open()) {
            get_logger().log(LogLevel::Error, std::format("Failed to open MCP config file for writing: {}", config_path_));
            return std::unexpected(MCPServerError::ConfigNotFound);
        }
        
        file << j.dump(2);
        get_logger().log(LogLevel::Info, std::format("Saved MCP config with {} servers", servers_.size()));
        return {};
        
    } catch (const std::exception& e) {
        get_logger().log(LogLevel::Error, std::format("Failed to save MCP config: {}", e.what()));
        return std::unexpected(MCPServerError::ConfigParseError);
    }
}

std::expected<MCPServerConfiguration, MCPServerError> MCPServerConfig::get_server(const std::string& name) const {
    auto it = servers_.find(name);
    if (it != servers_.end()) {
        return it->second;
    }
    return std::unexpected(MCPServerError::ServerNotFound);
}

void MCPServerConfig::add_server(const std::string& name, const MCPServerConfiguration& server) {
    servers_[name] = server;
    get_logger().log(LogLevel::Info, std::format("Added MCP server: {} ({})", name, server.description));
}

bool MCPServerConfig::remove_server(const std::string& name) {
    auto it = servers_.find(name);
    if (it != servers_.end()) {
        servers_.erase(it);
        get_logger().log(LogLevel::Info, std::format("Removed MCP server: {}", name));
        return true;
    }
    return false;
}

std::vector<std::string> MCPServerConfig::get_enabled_servers() const {
    std::vector<std::string> enabled;
    for (const auto& [name, server] : servers_) {
        if (server.enabled) {
            enabled.push_back(name);
        }
    }
    return enabled;
}

void MCPServerConfig::create_default_config() {
    servers_.clear();
    
    // Filesystem MCP server
    MCPServerConfiguration filesystem;
    filesystem.name = "filesystem";
    filesystem.command = "npx";
    filesystem.args = {"-y", "@modelcontextprotocol/server-filesystem", "/tmp"};
    filesystem.description = "Local filesystem access";
    filesystem.enabled = true;
    add_server("filesystem", filesystem);
    
    // GitHub MCP server
    MCPServerConfiguration github;
    github.name = "github";
    github.command = "npx";
    github.args = {"-y", "@modelcontextprotocol/server-github"};
    github.description = "GitHub repository access";
    github.enabled = false; // Requires GITHUB_PERSONAL_ACCESS_TOKEN
    add_server("github", github);
    
    // Brave Search MCP server
    MCPServerConfiguration brave;
    brave.name = "brave-search";
    brave.command = "npx";
    brave.args = {"-y", "@modelcontextprotocol/server-brave-search"};
    brave.description = "Web search via Brave Search API";
    brave.enabled = false; // Requires BRAVE_API_KEY
    add_server("brave-search", brave);
    
    // Sequential Thinking MCP server
    MCPServerConfiguration sequential;
    sequential.name = "sequential-thinking";
    sequential.command = "npx";
    sequential.args = {"-y", "@modelcontextprotocol/server-sequential-thinking"};
    sequential.description = "Step-by-step reasoning capabilities";
    sequential.enabled = true;
    add_server("sequential-thinking", sequential);
    
    // Playwright MCP server
    MCPServerConfiguration playwright;
    playwright.name = "playwright";
    playwright.command = "npx";
    playwright.args = {"-y", "@modelcontextprotocol/server-playwright"};
    playwright.description = "Web browser automation";
    playwright.enabled = false; // Optional, requires additional setup
    add_server("playwright", playwright);
    
    get_logger().log(LogLevel::Info, std::format("Created default MCP configuration with {} servers", servers_.size()));
}

std::string MCPServerConfig::error_to_string(MCPServerError error) const {
    switch (error) {
        case MCPServerError::ConfigNotFound: return "Config file not found";
        case MCPServerError::ConfigParseError: return "Config parsing error";
        case MCPServerError::ServerNotFound: return "Server not found";
        case MCPServerError::ConnectionError: return "Connection error";
        case MCPServerError::InitializationError: return "Initialization error";
        case MCPServerError::Unknown: return "Unknown error";
        default: return "Unhandled error";
    }
}