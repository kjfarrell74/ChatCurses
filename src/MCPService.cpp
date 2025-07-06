#include "MCPService.hpp"
#include "GlobalLogger.hpp"
#include <format>
#include <algorithm>
#include <cctype>
#include <regex>

void MCPService::configure(const std::string& server_url) {
    if (current_server_url_ == server_url && mcp_client_) {
        return; // Already configured
    }

    get_logger().log(LogLevel::Info, std::format("Configuring MCP service for: {}", server_url));
    
    current_server_url_ = server_url;
    mcp_client_ = std::make_unique<MCPClient>(server_url);
    cache_valid_ = false;
    
    launch_bridge_if_needed();
    
    // Try to establish connection and refresh cache
    refresh_cache();
}

bool MCPService::is_connected() const {
    bool connected = mcp_client_ && mcp_client_->get_connection_state() == MCPConnectionState::Connected;
    if (!connected) {
        get_logger().log(LogLevel::Debug, std::format("MCP not connected - client exists: {}, state: {}", 
            mcp_client_ ? "yes" : "no", 
            mcp_client_ ? static_cast<int>(mcp_client_->get_connection_state()) : -1));
    }
    return connected;
}

void MCPService::launch_bridge_if_needed() {
    // Launch bridge for known local servers
    if (current_server_url_ == "ws://localhost:9092") {
        // Check if bridge is already running by trying to connect
        get_logger().log(LogLevel::Info, "Checking if Brave search bridge is already running on port 9092");
        
        // Try to connect first - if it fails, then launch bridge
        // Note: This is a simplified check - in production you'd want more robust detection
        auto temp_client = std::make_unique<MCPClient>(current_server_url_);
        auto connect_result = temp_client->connect().get();
        
        if (!connect_result) {
            get_logger().log(LogLevel::Info, "Bridge not running, launching Brave search bridge on port 9092");
            mcp_client_->launch_websocketd_bridge("/home/kfarrell/mcp-servers/venv/bin/python /home/kfarrell/mcp-servers/brave-search-rate-limited.py", 9092);
        } else {
            get_logger().log(LogLevel::Info, "Bridge already running on port 9092");
            temp_client->disconnect().wait();
        }
    }
    // ScrapeX MCP Bridge
    if (current_server_url_ == "ws://localhost:9093") {
        get_logger().log(LogLevel::Info, "Checking if ScrapeX bridge is already running on port 9093");
        
        auto temp_client = std::make_unique<MCPClient>(current_server_url_);
        auto connect_result = temp_client->connect().get();
        
        if (!connect_result) {
            get_logger().log(LogLevel::Info, "Bridge not running, launching ScrapeX bridge on port 9093");
            mcp_client_->launch_websocketd_bridge("/home/kfarrell/mcp-servers/venv/bin/python /home/kfarrell/.config/Claude/mcp_scrapex_bridge_fastmcp.py", 9093);
        } else {
            get_logger().log(LogLevel::Info, "Bridge already running on port 9093");
            temp_client->disconnect().wait();
        }
    }
    // Add other known servers here
}

std::vector<nlohmann::json> MCPService::list_available_tools() {
    if (!mcp_client_) return {};
    
    if (!cache_valid_) {
        refresh_cache();
    }
    
    return tools_cache_;
}

std::optional<nlohmann::json> MCPService::call_tool(const std::string& name, const nlohmann::json& arguments) {
    if (!mcp_client_ || !mcp_client_->tool_manager()) return std::nullopt;
    
    try {
        auto result = mcp_client_->tool_manager()->call_tool(name, arguments);
        return result;
    } catch (const std::exception& e) {
        get_logger().log(LogLevel::Error, std::format("Error calling tool {}: {}", name, e.what()));
        return std::nullopt;
    }
}

std::vector<nlohmann::json> MCPService::list_available_resources() {
    if (!mcp_client_) return {};
    
    if (!cache_valid_) {
        refresh_cache();
    }
    
    return resources_cache_;
}

std::optional<nlohmann::json> MCPService::read_resource(const std::string& uri) {
    if (!mcp_client_ || !mcp_client_->resource_manager()) return std::nullopt;
    
    try {
        return mcp_client_->resource_manager()->read_resource(uri);
    } catch (const std::exception& e) {
        get_logger().log(LogLevel::Error, std::format("Error reading resource {}: {}", uri, e.what()));
        return std::nullopt;
    }
}

std::vector<nlohmann::json> MCPService::list_available_prompts() {
    if (!mcp_client_) return {};
    
    if (!cache_valid_) {
        refresh_cache();
    }
    
    return prompts_cache_;
}

std::optional<std::string> MCPService::get_prompt(const std::string& name, const nlohmann::json& arguments) {
    if (!mcp_client_ || !mcp_client_->prompt_manager()) return std::nullopt;
    
    try {
        return mcp_client_->prompt_manager()->get_prompt(name, arguments);
    } catch (const std::exception& e) {
        get_logger().log(LogLevel::Error, std::format("Error getting prompt {}: {}", name, e.what()));
        return std::nullopt;
    }
}

bool MCPService::should_use_tools(const std::string& user_message) {
    if (!mcp_client_) return false;
    
    // Convert to lowercase for matching
    std::string lower_msg = user_message;
    std::transform(lower_msg.begin(), lower_msg.end(), lower_msg.begin(), ::tolower);
    
    // Common patterns that suggest tool usage
    std::vector<std::string> patterns = {
        "scrape", "fetch", "get data from", "extract", "download",
        "search for", "find information", "look up", "retrieve",
        "weather", "temperature", "forecast",
        "map", "location", "directions", "address",
        "news", "latest", "current events",
        "file", "document", "read file", "save file",
        "youtube", "video", "transcript", "subtitles", "captions"
    };
    
    for (const auto& pattern : patterns) {
        if (lower_msg.find(pattern) != std::string::npos) {
            return true;
        }
    }
    
    // Check for URLs
    std::regex url_pattern(R"(https?://[^\s]+)");
    if (std::regex_search(user_message, url_pattern)) {
        return true;
    }
    
    return false;
}

std::string MCPService::get_tools_description() {
    auto tools = list_available_tools();
    if (tools.empty()) {
        return "";
    }
    
    std::string description = "\n\nAvailable tools:\n";
    for (const auto& tool : tools) {
        if (tool.contains("name") && tool.contains("description")) {
            description += std::format("- {}: {}\n", 
                tool["name"].get<std::string>(), 
                tool["description"].get<std::string>());
        }
    }
    
    return description;
}

void MCPService::set_notification_interface(MCPNotificationInterface* notifier) {
    if (mcp_client_ && mcp_client_->tool_manager()) {
        mcp_client_->tool_manager()->set_notification_interface(notifier);
    }
}

void MCPService::refresh_cache() {
    if (!mcp_client_) return;
    
    try {
        // Connect if not already connected
        if (mcp_client_->get_connection_state() != MCPConnectionState::Connected) {
            auto connect_result = mcp_client_->connect().get();
            if (!connect_result) {
                get_logger().log(LogLevel::Error, std::format("Failed to connect to MCP server: {}", connect_result.error().message));
                return;
            }
        }
        
        // Refresh caches
        if (mcp_client_->tool_manager()) {
            tools_cache_ = mcp_client_->tool_manager()->list_tools();
        }
        
        if (mcp_client_->resource_manager()) {
            resources_cache_ = mcp_client_->resource_manager()->list_resources();
        }
        
        if (mcp_client_->prompt_manager()) {
            prompts_cache_ = mcp_client_->prompt_manager()->list_prompts();
        }
        
        cache_valid_ = true;
        get_logger().log(LogLevel::Info, std::format("MCP cache refreshed: {} tools, {} resources, {} prompts", 
                                                    tools_cache_.size(), resources_cache_.size(), prompts_cache_.size()));
        
    } catch (const std::exception& e) {
        get_logger().log(LogLevel::Error, std::format("Error refreshing MCP cache: {}", e.what()));
    }
}