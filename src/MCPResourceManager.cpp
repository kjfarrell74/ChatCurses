#include "MCPResourceManager.hpp"
#include "MCPClient.hpp"
#include "MCPProtocol.hpp"
#include <future>

MCPResourceManager::MCPResourceManager(MCPClient* client)
    : client_(client) {}

MCPResourceManager::~MCPResourceManager() = default;

std::vector<nlohmann::json> MCPResourceManager::list_resources(std::optional<std::string> cursor) {
    // If cache is valid and no cursor, return cached
    if (!cursor.has_value() && !resource_cache_.empty()) {
        return resource_cache_;
    }
    auto request = MCPProtocolMessages::create_resources_list_request(cursor);
    auto fut = client_->send_request_for_manager(request);
    auto result = fut.get();
    if (!result || result->is_error() || !result->result.has_value()) {
        return {};
    }
    const auto& res = result->result.value();
    if (res.contains("resources") && res["resources"].is_array()) {
        resource_cache_ = res["resources"].get<std::vector<nlohmann::json>>();
        if (res.contains("cursor") && res["cursor"].is_string()) {
            last_cursor_ = res["cursor"].get<std::string>();
        }
        return resource_cache_;
    }
    return {};
}

std::optional<nlohmann::json> MCPResourceManager::read_resource(const std::string& uri) {
    auto request = MCPProtocolMessages::create_resources_read_request(uri);
    auto fut = client_->send_request_for_manager(request);
    auto result = fut.get();
    if (!result || result->is_error() || !result->result.has_value()) {
        return std::nullopt;
    }
    return result->result.value();
}

std::optional<std::string> MCPResourceManager::resolve_uri(const std::string& uri) {
    // For now, just return the URI (stub)
    return uri;
}

void MCPResourceManager::clear_cache() {
    resource_cache_.clear();
    last_cursor_.clear();
}

void MCPResourceManager::handle_list_changed_notification() {
    clear_cache();
} 