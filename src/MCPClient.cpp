#include "MCPClient.hpp"
#include "GlobalLogger.hpp"
#include <iostream>
#include <ixwebsocket/IXWebSocket.h>
#include <sstream>
#include <format>
#include "MCPResourceManager.hpp"
#include "MCPToolManager.hpp"
#include "MCPPromptManager.hpp"

MCPClient::MCPClient(const std::string& server_url)
    : server_url_(server_url) {
    ws_ = std::make_unique<ix::WebSocket>();
    resource_manager_ = std::make_unique<MCPResourceManager>(this);
    tool_manager_ = std::make_unique<MCPToolManager>(this);
    prompt_manager_ = std::make_unique<MCPPromptManager>(this);
    setup_websocket();
}

MCPClient::~MCPClient() {
    if (connection_state_ != MCPConnectionState::Disconnected) {
        disconnect().wait();
    }
    cleanup_websocket();
    if (bridge_thread_.joinable()) {
        bridge_thread_.join();
    }
    resource_manager_.reset();
    tool_manager_.reset();
    prompt_manager_.reset();
}

// AIClientInterface implementation
void MCPClient::set_api_key(const std::string& key) {
    std::lock_guard lock(mutex_);
    api_key_ = key;
}

void MCPClient::set_system_prompt(const std::string& prompt) {
    std::lock_guard lock(mutex_);
    system_prompt_ = prompt;
}

void MCPClient::set_model(const std::string& model) {
    std::lock_guard lock(mutex_);
    model_ = model;
}

void MCPClient::clear_history() {
    std::lock_guard lock(mutex_);
    conversation_history_.clear();
}

void MCPClient::push_user_message(const std::string& content) {
    std::lock_guard lock(mutex_);
    conversation_history_.push_back({{"role", "user"}, {"content", content}});
}

void MCPClient::push_assistant_message(const std::string& content) {
    std::lock_guard lock(mutex_);
    conversation_history_.push_back({{"role", "assistant"}, {"content", content}});
}

nlohmann::json MCPClient::build_message_history(const std::string& latest_user_message) const {
    std::lock_guard lock(mutex_);
    auto history = conversation_history_;
    if (!latest_user_message.empty()) {
        history.push_back({{"role", "user"}, {"content", latest_user_message}});
    }
    return history;
}

std::future<std::expected<std::string, ApiErrorInfo>> MCPClient::send_message(
    const nlohmann::json& messages,
    const std::string& model) {
    
    return std::async(std::launch::async, [this, messages, model]() -> std::expected<std::string, ApiErrorInfo> {
        // Ensure we're connected
        if (connection_state_ != MCPConnectionState::Connected) {
            auto connect_result = connect().get();
            if (!connect_result) {
                return std::unexpected(connect_result.error());
            }
        }
        
        // Check if server supports sampling
        if (!server_capabilities_ || !server_capabilities_->sampling.has_value()) {
            return std::unexpected(ApiErrorInfo{
                ApiError::FeatureNotSupported,
                "Server does not support sampling/createMessage"
            });
        }
        
        // Create sampling request
        std::string system_prompt;
        {
            std::lock_guard lock(mutex_);
            system_prompt = system_prompt_;
        }
        
        std::optional<nlohmann::json> system_prompt_json;
        if (!system_prompt.empty()) {
            system_prompt_json = system_prompt;
        }
        
        auto request = MCPProtocolMessages::create_sampling_create_message_request(
            messages, std::nullopt, system_prompt_json);
        
        // Send request and wait for response
        auto response_result = send_request(request).get();
        if (!response_result) {
            return std::unexpected(response_result.error());
        }
        
        auto response = response_result.value();
        if (response.is_error()) {
            return std::unexpected(mcp_error_to_api_error(response.error.value()));
        }
        
        // Extract response content
        if (!response.result.has_value()) {
            return std::unexpected(ApiErrorInfo{
                ApiError::InvalidResponse,
                "No result in sampling response"
            });
        }
        
        auto result = response.result.value();
        if (!result.contains("content") || !result["content"].is_string()) {
            return std::unexpected(ApiErrorInfo{
                ApiError::InvalidResponse,
                "Invalid sampling response format"
            });
        }
        
        return result["content"].get<std::string>();
    });
}

void MCPClient::send_message_stream(
    const std::string& prompt,
    const std::string& model,
    std::function<void(const std::string& chunk, bool is_last_chunk)> on_chunk_cb,
    std::function<void()> on_done_cb,
    std::function<void(const ApiErrorInfo& error)> on_error_cb) {
    
    // For now, use non-streaming approach
    // TODO: Implement proper streaming when MCP spec supports it
    std::thread([this, prompt, model, on_chunk_cb, on_done_cb, on_error_cb]() {
        nlohmann::json messages = nlohmann::json::array();
        messages.push_back({{"role", "user"}, {"content", prompt}});
        
        auto future_result = send_message(messages, model);
        auto result = future_result.get();
        
        if (result) {
            on_chunk_cb(result.value(), true);
            on_done_cb();
        } else {
            on_error_cb(result.error());
        }
    }).detach();
}

// MCP-specific methods
void MCPClient::set_server_url(const std::string& url) {
    std::lock_guard lock(mutex_);
    server_url_ = url;
}

void MCPClient::launch_websocketd_bridge(const std::string& mcp_cmd, int ws_port) {
    if (bridge_running_) return;
    
    bridge_running_ = true;
    bridge_thread_ = std::thread([this, mcp_cmd, ws_port]() {
        std::string cmd = std::format("websocketd --port={} {} > /tmp/websocketd_bridge.log 2>&1", ws_port, mcp_cmd);
        get_logger().log(LogLevel::Info, std::format("Starting MCP bridge: {}", cmd));
        std::system(cmd.c_str());
        bridge_running_ = false;
    });
}

// Connection management
std::future<std::expected<void, ApiErrorInfo>> MCPClient::connect() {
    return std::async(std::launch::async, [this]() -> std::expected<void, ApiErrorInfo> {
        if (connection_state_ != MCPConnectionState::Disconnected) {
            return std::unexpected(ApiErrorInfo{
                ApiError::InvalidState,
                "Already connected or connecting"
            });
        }
        
        connection_state_ = MCPConnectionState::Connecting;
        
        // Connect WebSocket
        ws_->setUrl(server_url_);
        auto connect_result = ws_->connect(10);
        if (!connect_result.success) {
            connection_state_ = MCPConnectionState::Error;
            return std::unexpected(ApiErrorInfo{
                ApiError::ConnectionError,
                std::format("Failed to connect to MCP server: {}", connect_result.errorStr)
            });
        }
        
        ws_->start();
        
        // Initialize MCP protocol
        auto init_result = initialize_connection().get();
        if (!init_result) {
            connection_state_ = MCPConnectionState::Error;
            ws_->stop();
            return std::unexpected(init_result.error());
        }
        
        connection_state_ = MCPConnectionState::Connected;
        get_logger().log(LogLevel::Info, "MCP connection established");
        
        return {};
    });
}

std::future<std::expected<void, ApiErrorInfo>> MCPClient::disconnect() {
    return std::async(std::launch::async, [this]() -> std::expected<void, ApiErrorInfo> {
        if (connection_state_ == MCPConnectionState::Disconnected) {
            return {};
        }
        
        connection_state_ = MCPConnectionState::Shutting_Down;
        
        // Send shutdown request
        try {
            auto shutdown_request = MCPProtocolMessages::create_shutdown_request();
            auto response_result = send_request(shutdown_request, std::chrono::milliseconds(5000)).get();
            // Don't fail on shutdown error - we're disconnecting anyway
        } catch (...) {
            // Ignore shutdown errors
        }
        
        ws_->stop();
        connection_state_ = MCPConnectionState::Disconnected;
        
        get_logger().log(LogLevel::Info, "MCP connection closed");
        return {};
    });
}

MCPConnectionState MCPClient::get_connection_state() const {
    return connection_state_;
}

std::optional<MCPCapabilities> MCPClient::get_server_capabilities() const {
    std::lock_guard lock(mutex_);
    return server_capabilities_;
}

// Private methods
std::future<std::expected<void, ApiErrorInfo>> MCPClient::initialize_connection() {
    return std::async(std::launch::async, [this]() -> std::expected<void, ApiErrorInfo> {
        connection_state_ = MCPConnectionState::Initializing;
        
        // Create client capabilities
        MCPCapabilities client_capabilities;
        client_capabilities.sampling = MCPCapabilities::Sampling{};
        
        // Create initialize parameters
        MCPInitializeParams init_params{
            MCP_PROTOCOL_VERSION,
            client_capabilities,
            MCPClientInfo{CLIENT_NAME, CLIENT_VERSION}
        };
        
        // Send initialize request
        auto init_request = MCPProtocolMessages::create_initialize_request(init_params);
        auto response_result = send_request(init_request).get();
        
        if (!response_result) {
            return std::unexpected(response_result.error());
        }
        
        auto response = response_result.value();
        if (response.is_error()) {
            return std::unexpected(mcp_error_to_api_error(response.error.value()));
        }
        
        // Parse initialize response
        auto result = response.result.value();
        auto init_result = MCPInitializeResult::from_json(result);
        if (!init_result) {
            return std::unexpected(ApiErrorInfo{
                ApiError::InvalidResponse,
                std::format("Invalid initialize response: {}", init_result.error())
            });
        }
        
        // Store server capabilities and info
        {
            std::lock_guard lock(mutex_);
            server_capabilities_ = init_result->capabilities;
            server_info_ = init_result->server_info;
        }
        
        // Send initialized notification
        auto initialized_notification = MCPProtocolMessages::create_initialized_notification();
        send_notification(initialized_notification);
        
        get_logger().log(LogLevel::Info, std::format("MCP initialized with server: {} v{}", 
                                                    init_result->server_info.name, 
                                                    init_result->server_info.version));
        
        return {};
    });
}

std::future<std::expected<MCPResponse, ApiErrorInfo>> MCPClient::send_request(
    const MCPRequest& request, 
    std::chrono::milliseconds timeout) {
    
    return std::async(std::launch::async, [this, request, timeout]() -> std::expected<MCPResponse, ApiErrorInfo> {
        std::string request_id = message_id_to_string(request.id);
        
        // Create promise for response
        std::promise<MCPResponse> response_promise;
        auto response_future = response_promise.get_future();
        
        {
            std::lock_guard lock(pending_requests_mutex_);
            pending_requests_[request_id] = std::move(response_promise);
        }
        
        // Send request
        auto message_json = request.to_json();
        std::string message_str = message_json.dump();
        
        get_logger().log(LogLevel::Debug, std::format("MCPClient::send_request - Sending: {}", message_str));
        ws_->send(message_str);
        
        // Wait for response
        auto status = response_future.wait_for(timeout);
        if (status == std::future_status::timeout) {
            std::lock_guard lock(pending_requests_mutex_);
            pending_requests_.erase(request_id);
            return std::unexpected(ApiErrorInfo{
                ApiError::Timeout,
                "Request timed out"
            });
        }
        
        auto response = response_future.get();
        get_logger().log(LogLevel::Debug, std::format("MCPClient::send_request - Response: {}", response.to_json().dump()));
        return response;
    });
}

void MCPClient::send_notification(const MCPNotification& notification) {
    auto message_json = notification.to_json();
    std::string message_str = message_json.dump();
    ws_->send(message_str);
}

void MCPClient::handle_message(const std::string& message) {
    try {
        auto json = nlohmann::json::parse(message);
        auto parsed_message = parse_mcp_message(json);
        
        if (!parsed_message) {
            get_logger().log(LogLevel::Error, std::format("Failed to parse MCP message: {}", parsed_message.error()));
            return;
        }
        
        auto& msg = parsed_message.value();
        
        if (msg->get_type() == "response") {
            auto response = dynamic_cast<MCPResponse*>(msg.get());
            if (response) {
                handle_response(*response);
            }
        } else if (msg->get_type() == "notification") {
            auto notification = dynamic_cast<MCPNotification*>(msg.get());
            if (notification) {
                handle_notification(*notification);
            }
        } else if (msg->get_type() == "request") {
            auto request = dynamic_cast<MCPRequest*>(msg.get());
            if (request) {
                handle_request(*request);
            }
        }
    } catch (const std::exception& e) {
        get_logger().log(LogLevel::Error, std::format("Error handling MCP message: {}", e.what()));
    }
}

void MCPClient::handle_response(const MCPResponse& response) {
    std::string response_id = message_id_to_string(response.id);
    
    std::lock_guard lock(pending_requests_mutex_);
    auto it = pending_requests_.find(response_id);
    if (it != pending_requests_.end()) {
        it->second.set_value(response);
        pending_requests_.erase(it);
    }
}

void MCPClient::handle_notification(const MCPNotification& notification) {
    get_logger().log(LogLevel::Debug, std::format("Received MCP notification: {}", notification.method));
    
    // Handle specific notifications
    if (notification.method == MCPMethods::RESOURCES_LIST_CHANGED) {
        if (resource_manager_) resource_manager_->handle_list_changed_notification();
    } else if (notification.method == MCPMethods::TOOLS_LIST_CHANGED) {
        if (tool_manager_) tool_manager_->handle_list_changed_notification();
    } else if (notification.method == MCPMethods::PROMPTS_LIST_CHANGED) {
        if (prompt_manager_) prompt_manager_->handle_list_changed_notification();
    }
}

void MCPClient::handle_request(const MCPRequest& request) {
    get_logger().log(LogLevel::Debug, std::format("Received MCP request: {}", request.method));
    
    // Handle ping requests
    if (request.method == MCPMethods::PING) {
        auto response = MCPProtocolMessages::create_ping_response(request.id);
        auto response_json = response.to_json();
        ws_->send(response_json.dump());
    }
    // Other request types can be handled here
}

void MCPClient::setup_websocket() {
    ws_->setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
        if (msg->type == ix::WebSocketMessageType::Message) {
            handle_message(msg->str);
        } else if (msg->type == ix::WebSocketMessageType::Error) {
            get_logger().log(LogLevel::Error, std::format("WebSocket error: {}", msg->errorInfo.reason));
            connection_state_ = MCPConnectionState::Error;
        } else if (msg->type == ix::WebSocketMessageType::Close) {
            get_logger().log(LogLevel::Info, "WebSocket connection closed");
            connection_state_ = MCPConnectionState::Disconnected;
        }
    });
}

void MCPClient::cleanup_websocket() {
    if (ws_) {
        ws_->stop();
        ws_.reset();
    }
}

// Helper methods
std::string MCPClient::message_id_to_string(const MCPMessageId& id) const {
    return std::visit([](const auto& value) -> std::string {
        if constexpr (std::is_same_v<std::decay_t<decltype(value)>, std::string>) {
            return value;
        } else {
            return std::to_string(value);
        }
    }, id);
}

ApiErrorInfo MCPClient::mcp_error_to_api_error(const MCPError& error) const {
    ApiError api_error;
    
    switch (error.code) {
        case MCPErrorCode::InvalidRequest:
            api_error = ApiError::InvalidRequest;
            break;
        case MCPErrorCode::MethodNotFound:
            api_error = ApiError::MethodNotFound;
            break;
        case MCPErrorCode::InvalidParams:
            api_error = ApiError::InvalidParams;
            break;
        case MCPErrorCode::InternalError:
            api_error = ApiError::InternalError;
            break;
        default:
            api_error = ApiError::Unknown;
            break;
    }
    
    return ApiErrorInfo{api_error, error.message};
}