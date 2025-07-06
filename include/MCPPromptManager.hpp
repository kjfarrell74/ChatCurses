#pragma once
#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

class MCPClient;

class MCPPromptManager {
public:
    explicit MCPPromptManager(MCPClient* client);
    ~MCPPromptManager();

    // List prompts (optionally paginated)
    std::vector<nlohmann::json> list_prompts(std::optional<std::string> cursor = std::nullopt);

    // Get a prompt by name with arguments
    std::optional<std::string> get_prompt(const std::string& name, std::optional<nlohmann::json> arguments = std::nullopt);

    // Render a prompt template
    std::string render_template(const std::string& template_str, const nlohmann::json& arguments);

    // Notification handler for prompt updates
    void handle_list_changed_notification();

private:
    MCPClient* client_;
    std::vector<nlohmann::json> prompt_cache_;
    std::string last_cursor_;
    // Add more as needed
    void clear_cache();
}; 