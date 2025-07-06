#pragma once
#include "MCPMessage.hpp"
#include <vector>
#include <unordered_map>
#include <string>

// MCP Protocol version
constexpr const char* MCP_PROTOCOL_VERSION = "2024-11-05";

// MCP Capability types
struct MCPCapabilities {
    // Client capabilities
    struct Roots {
        bool list_changed = false;
    };
    
    struct Sampling {
        // Sampling capabilities
    };
    
    // Server capabilities
    struct Logging {
        // Logging capabilities
    };
    
    struct Prompts {
        bool list_changed = false;
    };
    
    struct Resources {
        bool subscribe = false;
        bool list_changed = false;
    };
    
    struct Tools {
        bool list_changed = false;
    };
    
    std::optional<Roots> roots;
    std::optional<Sampling> sampling;
    std::optional<Logging> logging;
    std::optional<Prompts> prompts;
    std::optional<Resources> resources;
    std::optional<Tools> tools;
    
    nlohmann::json to_json() const;
    static std::expected<MCPCapabilities, std::string> from_json(const nlohmann::json& j);
};

// MCP Client info
struct MCPClientInfo {
    std::string name;
    std::string version;
    
    nlohmann::json to_json() const;
    static std::expected<MCPClientInfo, std::string> from_json(const nlohmann::json& j);
};

// MCP Server info
struct MCPServerInfo {
    std::string name;
    std::string version;
    
    nlohmann::json to_json() const;
    static std::expected<MCPServerInfo, std::string> from_json(const nlohmann::json& j);
};

// MCP Initialize request parameters
struct MCPInitializeParams {
    std::string protocol_version;
    MCPCapabilities capabilities;
    MCPClientInfo client_info;
    
    nlohmann::json to_json() const;
    static std::expected<MCPInitializeParams, std::string> from_json(const nlohmann::json& j);
};

// MCP Initialize response result
struct MCPInitializeResult {
    std::string protocol_version;
    MCPCapabilities capabilities;
    MCPServerInfo server_info;
    std::optional<std::string> instructions;
    
    nlohmann::json to_json() const;
    static std::expected<MCPInitializeResult, std::string> from_json(const nlohmann::json& j);
};

// MCP Protocol message factory
class MCPProtocolMessages {
public:
    // Create initialize request
    static MCPRequest create_initialize_request(const MCPInitializeParams& params);
    
    // Create initialize response
    static MCPResponse create_initialize_response(MCPMessageId request_id, const MCPInitializeResult& result);
    
    // Create initialized notification
    static MCPNotification create_initialized_notification();
    
    // Create shutdown request
    static MCPRequest create_shutdown_request();
    
    // Create shutdown response
    static MCPResponse create_shutdown_response(MCPMessageId request_id);
    
    // Create ping request
    static MCPRequest create_ping_request();
    
    // Create ping response
    static MCPResponse create_ping_response(MCPMessageId request_id);
    
    // Create resources/list request
    static MCPRequest create_resources_list_request(std::optional<std::string> cursor = std::nullopt);
    
    // Create tools/list request
    static MCPRequest create_tools_list_request(std::optional<std::string> cursor = std::nullopt);
    
    // Create prompts/list request
    static MCPRequest create_prompts_list_request(std::optional<std::string> cursor = std::nullopt);
    
    // Create resources/read request
    static MCPRequest create_resources_read_request(const std::string& uri);
    
    // Create tools/call request
    static MCPRequest create_tools_call_request(const std::string& name, 
                                               std::optional<nlohmann::json> arguments = std::nullopt);
    
    // Create prompts/get request
    static MCPRequest create_prompts_get_request(const std::string& name,
                                                std::optional<nlohmann::json> arguments = std::nullopt);
    
    // Create sampling/create_message request
    static MCPRequest create_sampling_create_message_request(const nlohmann::json& messages,
                                                           std::optional<nlohmann::json> model_preferences = std::nullopt,
                                                           std::optional<nlohmann::json> system_prompt = std::nullopt,
                                                           std::optional<bool> include_context = std::nullopt,
                                                           std::optional<double> temperature = std::nullopt,
                                                           std::optional<int> max_tokens = std::nullopt,
                                                           std::optional<std::vector<std::string>> stop_sequences = std::nullopt,
                                                           std::optional<nlohmann::json> metadata = std::nullopt);
};

// MCP Connection state
enum class MCPConnectionState {
    Disconnected,
    Connecting,
    Initializing,
    Connected,
    Shutting_Down,
    Error
};

// Convert state to string for logging
std::string to_string(MCPConnectionState state);

// MCP Protocol constants
namespace MCPMethods {
    constexpr const char* INITIALIZE = "initialize";
    constexpr const char* INITIALIZED = "initialized";
    constexpr const char* SHUTDOWN = "shutdown";
    constexpr const char* PING = "ping";
    constexpr const char* RESOURCES_LIST = "resources/list";
    constexpr const char* RESOURCES_READ = "resources/read";
    constexpr const char* RESOURCES_UPDATED = "resources/updated";
    constexpr const char* RESOURCES_LIST_CHANGED = "resources/list_changed";
    constexpr const char* TOOLS_LIST = "tools/list";
    constexpr const char* TOOLS_CALL = "tools/call";
    constexpr const char* TOOLS_LIST_CHANGED = "tools/list_changed";
    constexpr const char* PROMPTS_LIST = "prompts/list";
    constexpr const char* PROMPTS_GET = "prompts/get";
    constexpr const char* PROMPTS_LIST_CHANGED = "prompts/list_changed";
    constexpr const char* SAMPLING_CREATE_MESSAGE = "sampling/createMessage";
    constexpr const char* LOGGING_SET_LEVEL = "logging/setLevel";
    constexpr const char* ROOTS_LIST = "roots/list";
    constexpr const char* ROOTS_LIST_CHANGED = "roots/list_changed";
}