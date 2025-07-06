#pragma once
#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

class MCPClient;

class MCPResourceManager {
public:
    explicit MCPResourceManager(MCPClient* client);
    ~MCPResourceManager();

    // List resources (optionally paginated)
    std::vector<nlohmann::json> list_resources(std::optional<std::string> cursor = std::nullopt);

    // Read a resource by URI
    std::optional<nlohmann::json> read_resource(const std::string& uri);

    // Resolve a resource URI (if needed)
    std::optional<std::string> resolve_uri(const std::string& uri);

    // Cache management
    void clear_cache();

    // Notification handler for resource updates
    void handle_list_changed_notification();

private:
    MCPClient* client_;
    std::vector<nlohmann::json> resource_cache_;
    std::string last_cursor_;
    // Add more as needed
}; 