#pragma once
#include "AIClientInterface.hpp"
#include "MCPMessage.hpp"
#include "MCPProtocol.hpp"
#include <ixwebsocket/IXWebSocket.h>
#include <nlohmann/json.hpp>
#include <future>
#include <expected>
#include <string>
#include <atomic>
#include <thread>
#include <unordered_map>
#include <condition_variable>
#include <chrono>
#include "MCPResourceManager.hpp"
#include "MCPToolManager.hpp"
#include "MCPPromptManager.hpp"

class MCPClient : public AIClientInterface {
public:
    MCPClient(const std::string& server_url = "ws://localhost:3000");
    ~MCPClient();

    // AIClientInterface implementation
    void set_api_key(const std::string& key) override;
    void set_system_prompt(const std::string& prompt) override;
    void set_model(const std::string& model) override;
    void clear_history() override;
    void push_user_message(const std::string& content) override;
    void push_assistant_message(const std::string& content) override;
    nlohmann::json build_message_history(const std::string& latest_user_msg = "") const override;
    std::future<std::expected<std::string, ApiErrorInfo>> send_message(
        const nlohmann::json& messages,
        const std::string& model = "") override;

    void send_message_stream(
        const std::string& prompt,
        const std::string& model,
        std::function<void(const std::string& chunk, bool is_last_chunk)> on_chunk_cb,
        std::function<void()> on_done_cb,
        std::function<void(const ApiErrorInfo& error)> on_error_cb) override;

    // MCP-specific methods
    void set_server_url(const std::string& url);
    void launch_websocketd_bridge(const std::string& mcp_cmd, int ws_port = 8080);
    
    // Connection management
    std::future<std::expected<void, ApiErrorInfo>> connect();
    std::future<std::expected<void, ApiErrorInfo>> disconnect();
    
    // Get connection state
    MCPConnectionState get_connection_state() const;
    
    // Get server capabilities after successful connection
    std::optional<MCPCapabilities> get_server_capabilities() const;

    MCPResourceManager* resource_manager() { return resource_manager_.get(); }
    MCPToolManager* tool_manager() { return tool_manager_.get(); }
    MCPPromptManager* prompt_manager() { return prompt_manager_.get(); }

    // Allow managers to send requests
    std::future<std::expected<MCPResponse, ApiErrorInfo>> send_request_for_manager(const MCPRequest& request, 
                                                                                   std::chrono::milliseconds timeout = std::chrono::milliseconds(30000)) {
        return send_request(request, timeout);
    }

private:
    // Core MCP protocol methods
    std::future<std::expected<void, ApiErrorInfo>> initialize_connection();
    std::future<std::expected<MCPResponse, ApiErrorInfo>> send_request(const MCPRequest& request, 
                                                                       std::chrono::milliseconds timeout = std::chrono::milliseconds(30000));
    void send_notification(const MCPNotification& notification);
    
    // Message handling
    void handle_message(const std::string& message);
    void handle_response(const MCPResponse& response);
    void handle_notification(const MCPNotification& notification);
    void handle_request(const MCPRequest& request);
    
    // WebSocket management
    void setup_websocket();
    void cleanup_websocket();
    
    // State management
    std::unique_ptr<ix::WebSocket> ws_;
    std::string server_url_;
    std::string api_key_;
    std::string system_prompt_;
    std::string model_;
    std::vector<nlohmann::json> conversation_history_;
    mutable std::mutex mutex_;
    
    // Connection state
    std::atomic<MCPConnectionState> connection_state_{MCPConnectionState::Disconnected};
    std::optional<MCPCapabilities> server_capabilities_;
    std::optional<MCPServerInfo> server_info_;
    
    // Request/response tracking
    std::unordered_map<std::string, std::promise<MCPResponse>> pending_requests_;
    std::unordered_map<std::string, std::promise<MCPResponse>> pending_responses_;
    std::mutex pending_requests_mutex_;
    
    // Bridge process management
    std::thread bridge_thread_;
    std::atomic<bool> bridge_running_{false};
    
    // Client info
    static constexpr const char* CLIENT_NAME = "ChatCurses";
    static constexpr const char* CLIENT_VERSION = "1.0.0";
    
    // Helper methods
    std::string message_id_to_string(const MCPMessageId& id) const;
    ApiErrorInfo mcp_error_to_api_error(const MCPError& error) const;

    std::unique_ptr<MCPResourceManager> resource_manager_;
    std::unique_ptr<MCPToolManager> tool_manager_;
    std::unique_ptr<MCPPromptManager> prompt_manager_;
}; 