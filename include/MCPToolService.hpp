#pragma once
#include "MCPServerManager.hpp"
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <nlohmann/json.hpp>

struct MCPTool {
    std::string name;
    std::string description;
    nlohmann::json input_schema;
    std::string server_name;
    
    nlohmann::json to_json() const;
};

// Service for managing MCP tools across multiple servers
class MCPToolService {
public:
    static MCPToolService& instance() {
        static MCPToolService inst;
        return inst;
    }

    // Initialize with server manager
    void initialize(MCPServerManager* server_manager);
    
    // Tool discovery and management
    std::vector<MCPTool> get_all_available_tools();
    std::optional<MCPTool> find_tool(const std::string& tool_name);
    void refresh_tool_cache();
    
    // Tool calling
    std::optional<nlohmann::json> call_tool(const std::string& tool_name, const nlohmann::json& arguments);
    
    // AI integration helpers
    std::string get_tools_description_for_ai();
    std::vector<std::string> detect_tool_calls_in_message(const std::string& message);
    bool should_process_with_tools(const std::string& message);
    
    // Auto tool calling based on message content
    std::optional<nlohmann::json> auto_call_tools(const std::string& user_message);
    
private:
    MCPToolService() = default;
    ~MCPToolService() = default;
    MCPToolService(const MCPToolService&) = delete;
    MCPToolService& operator=(const MCPToolService&) = delete;
    
    MCPServerManager* server_manager_ = nullptr;
    std::vector<MCPTool> tool_cache_;
    bool cache_valid_ = false;
    
    // Tool discovery from individual servers
    std::vector<MCPTool> discover_tools_from_server(const std::string& server_name);
};