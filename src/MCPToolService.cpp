#include "MCPToolService.hpp"
#include "GlobalLogger.hpp"
#include <algorithm>
#include <regex>

nlohmann::json MCPTool::to_json() const {
    nlohmann::json j;
    j["name"] = name;
    j["description"] = description;
    j["input_schema"] = input_schema;
    j["server"] = server_name;
    return j;
}

void MCPToolService::initialize(MCPServerManager* server_manager) {
    server_manager_ = server_manager;
    cache_valid_ = false;
    get_logger().log(LogLevel::Info, "MCPToolService initialized");
}

std::vector<MCPTool> MCPToolService::get_all_available_tools() {
    if (!server_manager_) {
        return {};
    }
    
    if (!cache_valid_) {
        refresh_tool_cache();
    }
    
    return tool_cache_;
}

void MCPToolService::refresh_tool_cache() {
    if (!server_manager_) {
        return;
    }
    
    tool_cache_.clear();
    
    // Get all connected servers
    auto connected_servers = server_manager_->get_connected_servers();
    
    get_logger().log(LogLevel::Info, std::format("Discovering tools from {} connected MCP servers", connected_servers.size()));
    
    for (const auto& server_name : connected_servers) {
        auto tools = discover_tools_from_server(server_name);
        tool_cache_.insert(tool_cache_.end(), tools.begin(), tools.end());
        
        get_logger().log(LogLevel::Info, std::format("Found {} tools from server '{}'", tools.size(), server_name));
    }
    
    cache_valid_ = true;
    get_logger().log(LogLevel::Info, std::format("Total tools discovered: {}", tool_cache_.size()));
}

std::vector<MCPTool> MCPToolService::discover_tools_from_server(const std::string& server_name) {
    std::vector<MCPTool> tools;
    
    if (!server_manager_) {
        return tools;
    }
    
    auto client = server_manager_->get_client(server_name);
    if (!client || !client->tool_manager()) {
        get_logger().log(LogLevel::Warning, std::format("No tool manager available for server '{}'", server_name));
        return tools;
    }
    
    try {
        auto tool_list = client->tool_manager()->list_tools();
        
        for (const auto& tool_data : tool_list) {
            if (tool_data.contains("name")) {
                MCPTool tool;
                tool.name = tool_data["name"];
                tool.description = tool_data.value("description", "");
                tool.input_schema = tool_data.value("inputSchema", nlohmann::json{});
                tool.server_name = server_name;
                
                tools.push_back(tool);
            }
        }
        
    } catch (const std::exception& e) {
        get_logger().log(LogLevel::Error, std::format("Error discovering tools from server '{}': {}", server_name, e.what()));
    }
    
    return tools;
}

std::optional<MCPTool> MCPToolService::find_tool(const std::string& tool_name) {
    auto tools = get_all_available_tools();
    
    auto it = std::find_if(tools.begin(), tools.end(), 
        [&tool_name](const MCPTool& tool) {
            return tool.name == tool_name;
        });
    
    if (it != tools.end()) {
        return *it;
    }
    
    return std::nullopt;
}

std::optional<nlohmann::json> MCPToolService::call_tool(const std::string& tool_name, const nlohmann::json& arguments) {
    if (!server_manager_) {
        return std::nullopt;
    }
    
    // Find which server has this tool
    auto tool = find_tool(tool_name);
    if (!tool.has_value()) {
        get_logger().log(LogLevel::Warning, std::format("Tool '{}' not found", tool_name));
        return std::nullopt;
    }
    
    auto client = server_manager_->get_client(tool->server_name);
    if (!client || !client->tool_manager()) {
        get_logger().log(LogLevel::Error, std::format("No client available for server '{}'", tool->server_name));
        return std::nullopt;
    }
    
    try {
        get_logger().log(LogLevel::Info, std::format("Calling tool '{}' on server '{}'", tool_name, tool->server_name));
        
        auto result = client->tool_manager()->call_tool(tool_name, arguments);
        
        get_logger().log(LogLevel::Info, std::format("Tool '{}' executed successfully", tool_name));
        return result;
        
    } catch (const std::exception& e) {
        get_logger().log(LogLevel::Error, std::format("Error calling tool '{}': {}", tool_name, e.what()));
        return std::nullopt;
    }
}

std::string MCPToolService::get_tools_description_for_ai() {
    auto tools = get_all_available_tools();
    
    if (tools.empty()) {
        return "";
    }
    
    std::string description = "\n\nYou have access to the following MCP tools:\n\n";
    
    for (const auto& tool : tools) {
        description += std::format("**{}** (from {}): {}\n", tool.name, tool.server_name, tool.description);
        
        if (!tool.input_schema.empty() && tool.input_schema.contains("properties")) {
            description += "  Parameters: ";
            for (const auto& [param_name, param_info] : tool.input_schema["properties"].items()) {
                description += param_name + " ";
            }
            description += "\n";
        }
        description += "\n";
    }
    
    description += "When a user request would benefit from these tools, call them using the format: **TOOL_CALL: tool_name {\"param\": \"value\"}**\n";
    description += "You will receive the tool results and can incorporate them into your response.\n";
    
    return description;
}

std::vector<std::string> MCPToolService::detect_tool_calls_in_message(const std::string& message) {
    std::vector<std::string> tool_calls;
    
    // Look for patterns like "TOOL_CALL: tool_name {...}"
    std::regex tool_call_regex(R"(\*\*TOOL_CALL:\s*(\w+)\s*(\{[^}]*\})\*\*)");
    std::sregex_iterator iter(message.begin(), message.end(), tool_call_regex);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        std::smatch match = *iter;
        tool_calls.push_back(match.str());
    }
    
    return tool_calls;
}

bool MCPToolService::should_process_with_tools(const std::string& message) {
    // Simple heuristics to determine if tools might be useful
    std::vector<std::string> tool_keywords = {
        "search", "find", "file", "directory", "list", "read", "write",
        "browse", "web", "internet", "github", "repository", "code"
    };
    
    std::string lowercase_message = message;
    std::transform(lowercase_message.begin(), lowercase_message.end(), lowercase_message.begin(), ::tolower);
    
    for (const auto& keyword : tool_keywords) {
        if (lowercase_message.find(keyword) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

std::optional<nlohmann::json> MCPToolService::auto_call_tools(const std::string& user_message) {
    if (!should_process_with_tools(user_message)) {
        return std::nullopt;
    }
    
    auto tools = get_all_available_tools();
    if (tools.empty()) {
        return std::nullopt;
    }
    
    std::string lowercase_message = user_message;
    std::transform(lowercase_message.begin(), lowercase_message.end(), lowercase_message.begin(), ::tolower);
    
    // Auto-detect and call appropriate tools based on message content
    
    // File/directory operations
    if (lowercase_message.find("list files") != std::string::npos || 
        lowercase_message.find("files in") != std::string::npos ||
        lowercase_message.find("directory") != std::string::npos) {
        
        // Look for filesystem tools
        for (const auto& tool : tools) {
            if (tool.name.find("list") != std::string::npos || 
                tool.name.find("read") != std::string::npos ||
                tool.server_name == "filesystem") {
                
                nlohmann::json args;
                // Extract path if mentioned
                std::regex path_regex(R"((/[^\s]*)|([A-Za-z]:\\[^\s]*))");
                std::smatch path_match;
                if (std::regex_search(user_message, path_match, path_regex)) {
                    args["path"] = path_match.str();
                } else {
                    args["path"] = "."; // Current directory
                }
                
                get_logger().log(LogLevel::Info, std::format("Auto-calling tool '{}' for file operation", tool.name));
                return call_tool(tool.name, args);
            }
        }
    }
    
    // Web search operations
    if (lowercase_message.find("search") != std::string::npos && 
        (lowercase_message.find("web") != std::string::npos || 
         lowercase_message.find("internet") != std::string::npos ||
         lowercase_message.find("online") != std::string::npos)) {
        
        for (const auto& tool : tools) {
            if (tool.name.find("search") != std::string::npos || 
                tool.server_name == "brave-search") {
                
                nlohmann::json args;
                args["query"] = user_message;
                
                get_logger().log(LogLevel::Info, std::format("Auto-calling tool '{}' for web search", tool.name));
                return call_tool(tool.name, args);
            }
        }
    }
    
    return std::nullopt;
}