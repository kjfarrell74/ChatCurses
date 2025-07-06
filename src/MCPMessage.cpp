#include "MCPMessage.hpp"
#include <atomic>
#include <random>
#include <sstream>
#include <format>

// Static member initialization
std::atomic<int64_t> MCPMessage::id_counter_{1};

// MCPError implementation
nlohmann::json MCPError::to_json() const {
    nlohmann::json j;
    j["code"] = static_cast<int>(code);
    j["message"] = message;
    if (data.has_value()) {
        j["data"] = data.value();
    }
    return j;
}

std::expected<MCPError, std::string> MCPError::from_json(const nlohmann::json& j) {
    if (!j.is_object()) {
        return std::unexpected("Error must be an object");
    }
    
    if (!j.contains("code") || !j["code"].is_number_integer()) {
        return std::unexpected("Error must contain integer 'code' field");
    }
    
    if (!j.contains("message") || !j["message"].is_string()) {
        return std::unexpected("Error must contain string 'message' field");
    }
    
    auto code = static_cast<MCPErrorCode>(j["code"].get<int>());
    auto message = j["message"].get<std::string>();
    
    std::optional<nlohmann::json> data;
    if (j.contains("data")) {
        data = j["data"];
    }
    
    return MCPError(code, message, data);
}

// MCPMessage implementation
MCPMessageId MCPMessage::generate_id() {
    return id_counter_.fetch_add(1);
}

// MCPRequest implementation
nlohmann::json MCPRequest::to_json() const {
    nlohmann::json j;
    j["jsonrpc"] = jsonrpc;
    j["id"] = message_id_to_json(id);
    j["method"] = method;
    if (params.has_value()) {
        j["params"] = params.value();
    }
    return j;
}

std::expected<MCPRequest, std::string> MCPRequest::from_json(const nlohmann::json& j) {
    if (!j.is_object()) {
        return std::unexpected("Request must be an object");
    }
    
    if (!j.contains("jsonrpc") || j["jsonrpc"] != "2.0") {
        return std::unexpected("Request must have jsonrpc field with value '2.0'");
    }
    
    if (!j.contains("id")) {
        return std::unexpected("Request must contain 'id' field");
    }
    
    if (!j.contains("method") || !j["method"].is_string()) {
        return std::unexpected("Request must contain string 'method' field");
    }
    
    auto id_result = message_id_from_json(j["id"]);
    if (!id_result) {
        return std::unexpected("Invalid id field: " + id_result.error());
    }
    
    auto method = j["method"].get<std::string>();
    
    std::optional<nlohmann::json> params;
    if (j.contains("params")) {
        params = j["params"];
    }
    
    return MCPRequest(id_result.value(), method, params);
}

// MCPResponse implementation
nlohmann::json MCPResponse::to_json() const {
    nlohmann::json j;
    j["jsonrpc"] = jsonrpc;
    j["id"] = message_id_to_json(id);
    
    if (result.has_value()) {
        j["result"] = result.value();
    } else if (error.has_value()) {
        j["error"] = error.value().to_json();
    }
    
    return j;
}

std::expected<MCPResponse, std::string> MCPResponse::from_json(const nlohmann::json& j) {
    if (!j.is_object()) {
        return std::unexpected("Response must be an object");
    }
    
    if (!j.contains("jsonrpc") || j["jsonrpc"] != "2.0") {
        return std::unexpected("Response must have jsonrpc field with value '2.0'");
    }
    
    if (!j.contains("id")) {
        return std::unexpected("Response must contain 'id' field");
    }
    
    auto id_result = message_id_from_json(j["id"]);
    if (!id_result) {
        return std::unexpected("Invalid id field: " + id_result.error());
    }
    
    bool has_result = j.contains("result");
    bool has_error = j.contains("error");
    
    if (!has_result && !has_error) {
        return std::unexpected("Response must contain either 'result' or 'error' field");
    }
    
    if (has_result && has_error) {
        return std::unexpected("Response cannot contain both 'result' and 'error' fields");
    }
    
    if (has_result) {
        return MCPResponse(id_result.value(), j["result"]);
    } else {
        auto error_result = MCPError::from_json(j["error"]);
        if (!error_result) {
            return std::unexpected("Invalid error field: " + error_result.error());
        }
        return MCPResponse(id_result.value(), error_result.value());
    }
}

// MCPNotification implementation
nlohmann::json MCPNotification::to_json() const {
    nlohmann::json j;
    j["jsonrpc"] = jsonrpc;
    j["method"] = method;
    if (params.has_value()) {
        j["params"] = params.value();
    }
    return j;
}

std::expected<MCPNotification, std::string> MCPNotification::from_json(const nlohmann::json& j) {
    if (!j.is_object()) {
        return std::unexpected("Notification must be an object");
    }
    
    if (!j.contains("jsonrpc") || j["jsonrpc"] != "2.0") {
        return std::unexpected("Notification must have jsonrpc field with value '2.0'");
    }
    
    if (j.contains("id")) {
        return std::unexpected("Notification must not contain 'id' field");
    }
    
    if (!j.contains("method") || !j["method"].is_string()) {
        return std::unexpected("Notification must contain string 'method' field");
    }
    
    auto method = j["method"].get<std::string>();
    
    std::optional<nlohmann::json> params;
    if (j.contains("params")) {
        params = j["params"];
    }
    
    return MCPNotification(method, params);
}

// Helper function implementations
std::expected<std::unique_ptr<MCPMessage>, std::string> parse_mcp_message(const nlohmann::json& j) {
    if (!j.is_object()) {
        return std::unexpected("Message must be an object");
    }
    
    if (!j.contains("jsonrpc") || j["jsonrpc"] != "2.0") {
        return std::unexpected("Message must have jsonrpc field with value '2.0'");
    }
    
    bool has_id = j.contains("id");
    bool has_method = j.contains("method");
    bool has_result = j.contains("result");
    bool has_error = j.contains("error");
    
    if (has_method && !has_id) {
        // This is a notification
        auto notification_result = MCPNotification::from_json(j);
        if (!notification_result) {
            return std::unexpected(notification_result.error());
        }
        return std::make_unique<MCPNotification>(std::move(notification_result.value()));
    } else if (has_method && has_id) {
        // This is a request
        auto request_result = MCPRequest::from_json(j);
        if (!request_result) {
            return std::unexpected(request_result.error());
        }
        return std::make_unique<MCPRequest>(std::move(request_result.value()));
    } else if (has_id && (has_result || has_error)) {
        // This is a response
        auto response_result = MCPResponse::from_json(j);
        if (!response_result) {
            return std::unexpected(response_result.error());
        }
        return std::make_unique<MCPResponse>(std::move(response_result.value()));
    } else {
        return std::unexpected("Invalid message format");
    }
}

nlohmann::json message_id_to_json(const MCPMessageId& id) {
    return std::visit([](const auto& value) -> nlohmann::json {
        return value;
    }, id);
}

std::expected<MCPMessageId, std::string> message_id_from_json(const nlohmann::json& j) {
    if (j.is_string()) {
        return j.get<std::string>();
    } else if (j.is_number_integer()) {
        return j.get<int64_t>();
    } else {
        return std::unexpected("Message ID must be string or integer");
    }
}