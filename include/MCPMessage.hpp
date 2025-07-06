#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <optional>
#include <variant>
#include <expected>

// MCP Error codes as defined in the specification
enum class MCPErrorCode {
    // JSON-RPC 2.0 standard errors
    ParseError = -32700,
    InvalidRequest = -32600,
    MethodNotFound = -32601,
    InvalidParams = -32602,
    InternalError = -32603,
    
    // MCP-specific errors
    InvalidMessageType = -32000,
    InvalidCapabilities = -32001,
    InvalidServerState = -32002,
    ResourceNotFound = -32003,
    ToolNotFound = -32004,
    PromptNotFound = -32005
};

// MCP Error structure
struct MCPError {
    MCPErrorCode code;
    std::string message;
    std::optional<nlohmann::json> data;
    
    MCPError(MCPErrorCode c, const std::string& msg, std::optional<nlohmann::json> d = std::nullopt)
        : code(c), message(msg), data(d) {}
    
    nlohmann::json to_json() const;
    static std::expected<MCPError, std::string> from_json(const nlohmann::json& j);
};

// Message ID type - can be string or integer
using MCPMessageId = std::variant<std::string, int64_t>;

// Base class for all MCP messages
class MCPMessage {
public:
    std::string jsonrpc = "2.0";
    
    virtual ~MCPMessage() = default;
    virtual nlohmann::json to_json() const = 0;
    virtual std::string get_type() const = 0;
    
    // Helper to generate unique message IDs
    static MCPMessageId generate_id();
    
protected:
    MCPMessage() = default;
    
private:
    static std::atomic<int64_t> id_counter_;
};

// JSON-RPC 2.0 Request message
class MCPRequest : public MCPMessage {
public:
    MCPMessageId id;
    std::string method;
    std::optional<nlohmann::json> params;
    
    MCPRequest(const std::string& method_name, std::optional<nlohmann::json> parameters = std::nullopt)
        : id(generate_id()), method(method_name), params(parameters) {}
    
    MCPRequest(MCPMessageId request_id, const std::string& method_name, std::optional<nlohmann::json> parameters = std::nullopt)
        : id(request_id), method(method_name), params(parameters) {}
    
    nlohmann::json to_json() const override;
    std::string get_type() const override { return "request"; }
    
    static std::expected<MCPRequest, std::string> from_json(const nlohmann::json& j);
};

// JSON-RPC 2.0 Response message
class MCPResponse : public MCPMessage {
public:
    MCPMessageId id;
    std::optional<nlohmann::json> result;
    std::optional<MCPError> error;
    
    // Success response
    MCPResponse(MCPMessageId request_id, nlohmann::json response_result)
        : id(request_id), result(response_result) {}
    
    // Error response
    MCPResponse(MCPMessageId request_id, MCPError response_error)
        : id(request_id), error(response_error) {}
    
    nlohmann::json to_json() const override;
    std::string get_type() const override { return "response"; }
    
    bool is_error() const { return error.has_value(); }
    bool is_success() const { return result.has_value(); }
    
    static std::expected<MCPResponse, std::string> from_json(const nlohmann::json& j);
};

// JSON-RPC 2.0 Notification message
class MCPNotification : public MCPMessage {
public:
    std::string method;
    std::optional<nlohmann::json> params;
    
    MCPNotification(const std::string& method_name, std::optional<nlohmann::json> parameters = std::nullopt)
        : method(method_name), params(parameters) {}
    
    nlohmann::json to_json() const override;
    std::string get_type() const override { return "notification"; }
    
    static std::expected<MCPNotification, std::string> from_json(const nlohmann::json& j);
};

// Helper function to parse any MCP message from JSON
std::expected<std::unique_ptr<MCPMessage>, std::string> parse_mcp_message(const nlohmann::json& j);

// Helper function to serialize message ID to JSON
nlohmann::json message_id_to_json(const MCPMessageId& id);

// Helper function to parse message ID from JSON
std::expected<MCPMessageId, std::string> message_id_from_json(const nlohmann::json& j);