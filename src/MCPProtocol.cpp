#include "MCPProtocol.hpp"
#include <format>

// MCPCapabilities implementation
nlohmann::json MCPCapabilities::to_json() const {
    nlohmann::json j;
    
    if (roots.has_value()) {
        j["roots"]["listChanged"] = roots->list_changed;
    }
    
    if (sampling.has_value()) {
        j["sampling"] = nlohmann::json::object();
    }
    
    if (logging.has_value()) {
        j["logging"] = nlohmann::json::object();
    }
    
    if (prompts.has_value()) {
        j["prompts"]["listChanged"] = prompts->list_changed;
    }
    
    if (resources.has_value()) {
        j["resources"]["subscribe"] = resources->subscribe;
        j["resources"]["listChanged"] = resources->list_changed;
    }
    
    if (tools.has_value()) {
        j["tools"]["listChanged"] = tools->list_changed;
    }
    
    return j;
}

std::expected<MCPCapabilities, std::string> MCPCapabilities::from_json(const nlohmann::json& j) {
    MCPCapabilities caps;
    
    if (j.contains("roots")) {
        Roots roots_cap;
        if (j["roots"].contains("listChanged") && j["roots"]["listChanged"].is_boolean()) {
            roots_cap.list_changed = j["roots"]["listChanged"].get<bool>();
        }
        caps.roots = roots_cap;
    }
    
    if (j.contains("sampling")) {
        caps.sampling = Sampling{};
    }
    
    if (j.contains("logging")) {
        caps.logging = Logging{};
    }
    
    if (j.contains("prompts")) {
        Prompts prompts_cap;
        if (j["prompts"].contains("listChanged") && j["prompts"]["listChanged"].is_boolean()) {
            prompts_cap.list_changed = j["prompts"]["listChanged"].get<bool>();
        }
        caps.prompts = prompts_cap;
    }
    
    if (j.contains("resources")) {
        Resources resources_cap;
        if (j["resources"].contains("subscribe") && j["resources"]["subscribe"].is_boolean()) {
            resources_cap.subscribe = j["resources"]["subscribe"].get<bool>();
        }
        if (j["resources"].contains("listChanged") && j["resources"]["listChanged"].is_boolean()) {
            resources_cap.list_changed = j["resources"]["listChanged"].get<bool>();
        }
        caps.resources = resources_cap;
    }
    
    if (j.contains("tools")) {
        Tools tools_cap;
        if (j["tools"].contains("listChanged") && j["tools"]["listChanged"].is_boolean()) {
            tools_cap.list_changed = j["tools"]["listChanged"].get<bool>();
        }
        caps.tools = tools_cap;
    }
    
    return caps;
}

// MCPClientInfo implementation
nlohmann::json MCPClientInfo::to_json() const {
    nlohmann::json j;
    j["name"] = name;
    j["version"] = version;
    return j;
}

std::expected<MCPClientInfo, std::string> MCPClientInfo::from_json(const nlohmann::json& j) {
    if (!j.is_object()) {
        return std::unexpected("ClientInfo must be an object");
    }
    
    if (!j.contains("name") || !j["name"].is_string()) {
        return std::unexpected("ClientInfo must contain string 'name' field");
    }
    
    if (!j.contains("version") || !j["version"].is_string()) {
        return std::unexpected("ClientInfo must contain string 'version' field");
    }
    
    return MCPClientInfo{
        j["name"].get<std::string>(),
        j["version"].get<std::string>()
    };
}

// MCPServerInfo implementation
nlohmann::json MCPServerInfo::to_json() const {
    nlohmann::json j;
    j["name"] = name;
    j["version"] = version;
    return j;
}

std::expected<MCPServerInfo, std::string> MCPServerInfo::from_json(const nlohmann::json& j) {
    if (!j.is_object()) {
        return std::unexpected("ServerInfo must be an object");
    }
    
    if (!j.contains("name") || !j["name"].is_string()) {
        return std::unexpected("ServerInfo must contain string 'name' field");
    }
    
    if (!j.contains("version") || !j["version"].is_string()) {
        return std::unexpected("ServerInfo must contain string 'version' field");
    }
    
    return MCPServerInfo{
        j["name"].get<std::string>(),
        j["version"].get<std::string>()
    };
}

// MCPInitializeParams implementation
nlohmann::json MCPInitializeParams::to_json() const {
    nlohmann::json j;
    j["protocolVersion"] = protocol_version;
    j["capabilities"] = capabilities.to_json();
    j["clientInfo"] = client_info.to_json();
    return j;
}

std::expected<MCPInitializeParams, std::string> MCPInitializeParams::from_json(const nlohmann::json& j) {
    if (!j.is_object()) {
        return std::unexpected("InitializeParams must be an object");
    }
    
    if (!j.contains("protocolVersion") || !j["protocolVersion"].is_string()) {
        return std::unexpected("InitializeParams must contain string 'protocolVersion' field");
    }
    
    if (!j.contains("capabilities")) {
        return std::unexpected("InitializeParams must contain 'capabilities' field");
    }
    
    if (!j.contains("clientInfo")) {
        return std::unexpected("InitializeParams must contain 'clientInfo' field");
    }
    
    auto capabilities_result = MCPCapabilities::from_json(j["capabilities"]);
    if (!capabilities_result) {
        return std::unexpected("Invalid capabilities: " + capabilities_result.error());
    }
    
    auto client_info_result = MCPClientInfo::from_json(j["clientInfo"]);
    if (!client_info_result) {
        return std::unexpected("Invalid clientInfo: " + client_info_result.error());
    }
    
    return MCPInitializeParams{
        j["protocolVersion"].get<std::string>(),
        capabilities_result.value(),
        client_info_result.value()
    };
}

// MCPInitializeResult implementation
nlohmann::json MCPInitializeResult::to_json() const {
    nlohmann::json j;
    j["protocolVersion"] = protocol_version;
    j["capabilities"] = capabilities.to_json();
    j["serverInfo"] = server_info.to_json();
    if (instructions.has_value()) {
        j["instructions"] = instructions.value();
    }
    return j;
}

std::expected<MCPInitializeResult, std::string> MCPInitializeResult::from_json(const nlohmann::json& j) {
    if (!j.is_object()) {
        return std::unexpected("InitializeResult must be an object");
    }
    
    if (!j.contains("protocolVersion") || !j["protocolVersion"].is_string()) {
        return std::unexpected("InitializeResult must contain string 'protocolVersion' field");
    }
    
    if (!j.contains("capabilities")) {
        return std::unexpected("InitializeResult must contain 'capabilities' field");
    }
    
    if (!j.contains("serverInfo")) {
        return std::unexpected("InitializeResult must contain 'serverInfo' field");
    }
    
    auto capabilities_result = MCPCapabilities::from_json(j["capabilities"]);
    if (!capabilities_result) {
        return std::unexpected("Invalid capabilities: " + capabilities_result.error());
    }
    
    auto server_info_result = MCPServerInfo::from_json(j["serverInfo"]);
    if (!server_info_result) {
        return std::unexpected("Invalid serverInfo: " + server_info_result.error());
    }
    
    std::optional<std::string> instructions;
    if (j.contains("instructions") && j["instructions"].is_string()) {
        instructions = j["instructions"].get<std::string>();
    }
    
    return MCPInitializeResult{
        j["protocolVersion"].get<std::string>(),
        capabilities_result.value(),
        server_info_result.value(),
        instructions
    };
}

// MCPProtocolMessages implementation
MCPRequest MCPProtocolMessages::create_initialize_request(const MCPInitializeParams& params) {
    return MCPRequest(MCPMethods::INITIALIZE, std::optional<nlohmann::json>(params.to_json()));
}

MCPResponse MCPProtocolMessages::create_initialize_response(MCPMessageId request_id, const MCPInitializeResult& result) {
    return MCPResponse(request_id, result.to_json());
}

MCPNotification MCPProtocolMessages::create_initialized_notification() {
    return MCPNotification(MCPMethods::INITIALIZED);
}

MCPRequest MCPProtocolMessages::create_shutdown_request() {
    return MCPRequest(MCPMethods::SHUTDOWN);
}

MCPResponse MCPProtocolMessages::create_shutdown_response(MCPMessageId request_id) {
    return MCPResponse(request_id, nlohmann::json::object());
}

MCPRequest MCPProtocolMessages::create_ping_request() {
    return MCPRequest(MCPMethods::PING);
}

MCPResponse MCPProtocolMessages::create_ping_response(MCPMessageId request_id) {
    return MCPResponse(request_id, nlohmann::json::object());
}

MCPRequest MCPProtocolMessages::create_resources_list_request(std::optional<std::string> cursor) {
    nlohmann::json params = nlohmann::json::object();
    if (cursor.has_value()) {
        params["cursor"] = cursor.value();
    }
    return MCPRequest(MCPMethods::RESOURCES_LIST, std::optional<nlohmann::json>(params));
}

MCPRequest MCPProtocolMessages::create_tools_list_request(std::optional<std::string> cursor) {
    nlohmann::json params = nlohmann::json::object();
    if (cursor.has_value()) {
        params["cursor"] = cursor.value();
    }
    return MCPRequest(MCPMethods::TOOLS_LIST, std::optional<nlohmann::json>(params));
}

MCPRequest MCPProtocolMessages::create_prompts_list_request(std::optional<std::string> cursor) {
    nlohmann::json params = nlohmann::json::object();
    if (cursor.has_value()) {
        params["cursor"] = cursor.value();
    }
    return MCPRequest(MCPMethods::PROMPTS_LIST, std::optional<nlohmann::json>(params));
}

MCPRequest MCPProtocolMessages::create_resources_read_request(const std::string& uri) {
    nlohmann::json params;
    params["uri"] = uri;
    return MCPRequest(MCPMethods::RESOURCES_READ, std::optional<nlohmann::json>(params));
}

MCPRequest MCPProtocolMessages::create_tools_call_request(const std::string& name, 
                                                         std::optional<nlohmann::json> arguments) {
    nlohmann::json params;
    params["name"] = name;
    if (arguments.has_value()) {
        params["arguments"] = arguments.value();
    }
    return MCPRequest(MCPMethods::TOOLS_CALL, std::optional<nlohmann::json>(params));
}

MCPRequest MCPProtocolMessages::create_prompts_get_request(const std::string& name,
                                                          std::optional<nlohmann::json> arguments) {
    nlohmann::json params;
    params["name"] = name;
    if (arguments.has_value()) {
        params["arguments"] = arguments.value();
    }
    return MCPRequest(MCPMethods::PROMPTS_GET, std::optional<nlohmann::json>(params));
}

MCPRequest MCPProtocolMessages::create_sampling_create_message_request(const nlohmann::json& messages,
                                                                      std::optional<nlohmann::json> model_preferences,
                                                                      std::optional<nlohmann::json> system_prompt,
                                                                      std::optional<bool> include_context,
                                                                      std::optional<double> temperature,
                                                                      std::optional<int> max_tokens,
                                                                      std::optional<std::vector<std::string>> stop_sequences,
                                                                      std::optional<nlohmann::json> metadata) {
    nlohmann::json params;
    params["messages"] = messages;
    
    if (model_preferences.has_value()) {
        params["modelPreferences"] = model_preferences.value();
    }
    
    if (system_prompt.has_value()) {
        params["systemPrompt"] = system_prompt.value();
    }
    
    if (include_context.has_value()) {
        params["includeContext"] = include_context.value();
    }
    
    if (temperature.has_value()) {
        params["temperature"] = temperature.value();
    }
    
    if (max_tokens.has_value()) {
        params["maxTokens"] = max_tokens.value();
    }
    
    if (stop_sequences.has_value()) {
        params["stopSequences"] = stop_sequences.value();
    }
    
    if (metadata.has_value()) {
        params["metadata"] = metadata.value();
    }
    
    return MCPRequest(MCPMethods::SAMPLING_CREATE_MESSAGE, std::optional<nlohmann::json>(params));
}

// Helper function
std::string to_string(MCPConnectionState state) {
    switch (state) {
        case MCPConnectionState::Disconnected: return "Disconnected";
        case MCPConnectionState::Connecting: return "Connecting";
        case MCPConnectionState::Initializing: return "Initializing";
        case MCPConnectionState::Connected: return "Connected";
        case MCPConnectionState::Shutting_Down: return "Shutting_Down";
        case MCPConnectionState::Error: return "Error";
        default: return "Unknown";
    }
}