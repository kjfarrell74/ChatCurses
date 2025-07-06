#include "MCPPromptManager.hpp"
#include "MCPClient.hpp"
#include "MCPProtocol.hpp"
#include <future>

MCPPromptManager::MCPPromptManager(MCPClient* client)
    : client_(client) {}

MCPPromptManager::~MCPPromptManager() = default;

std::vector<nlohmann::json> MCPPromptManager::list_prompts(std::optional<std::string> cursor) {
    if (!cursor.has_value() && !prompt_cache_.empty()) {
        return prompt_cache_;
    }
    auto request = MCPProtocolMessages::create_prompts_list_request(cursor);
    auto fut = client_->send_request_for_manager(request);
    auto result = fut.get();
    if (!result || result->is_error() || !result->result.has_value()) {
        return {};
    }
    const auto& res = result->result.value();
    if (res.contains("prompts") && res["prompts"].is_array()) {
        prompt_cache_ = res["prompts"].get<std::vector<nlohmann::json>>();
        if (res.contains("cursor") && res["cursor"].is_string()) {
            last_cursor_ = res["cursor"].get<std::string>();
        }
        return prompt_cache_;
    }
    return {};
}

std::optional<std::string> MCPPromptManager::get_prompt(const std::string& name, std::optional<nlohmann::json> arguments) {
    auto request = MCPProtocolMessages::create_prompts_get_request(name, arguments);
    auto fut = client_->send_request_for_manager(request);
    auto result = fut.get();
    if (!result || result->is_error() || !result->result.has_value()) {
        return std::nullopt;
    }
    const auto& res = result->result.value();
    if (res.contains("prompt") && res["prompt"].is_string()) {
        return res["prompt"].get<std::string>();
    }
    return std::nullopt;
}

std::string MCPPromptManager::render_template(const std::string& template_str, const nlohmann::json& arguments) {
    // TODO: Implement template rendering (simple stub)
    return template_str;
}

void MCPPromptManager::handle_list_changed_notification() {
    clear_cache();
}

void MCPPromptManager::clear_cache() {
    prompt_cache_.clear();
    last_cursor_.clear();
} 