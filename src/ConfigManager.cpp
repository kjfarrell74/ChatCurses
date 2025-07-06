#include "ConfigManager.hpp"
#include <fstream>
#include <stdexcept> // For exception handling
#include <nlohmann/json.hpp>

ConfigManager::ConfigManager(const std::string& config_path) : config_path_(config_path) {}

std::expected<Settings, ConfigError> ConfigManager::load() {
    std::ifstream ifs(config_path_);
    if (!ifs) {
        return std::unexpected(ConfigError::FileNotFound);
    }
    try {
        nlohmann::json j;
        ifs >> j;
        Settings settings;
        settings.user_display_name = j.value("user_display_name", "User");
        settings.system_prompt = j.value("system_prompt", "");
        settings.xai_api_key = j.value("xai_api_key", "");
        settings.claude_api_key = j.value("claude_api_key", "");
        settings.openai_api_key = j.value("openai_api_key", "");
        settings.gemini_api_key = j.value("gemini_api_key", "");
        settings.provider = j.value("provider", "xai");
        settings.model = j.value("model", settings.provider == "xai" ? "grok-3-beta" : "claude");
        settings.store_chat_history = j.value("store_chat_history", true);
        settings.theme_id = j.value("theme_id", 0);
        settings.mcp_server_url = j.value("mcp_server_url", "ws://localhost:9092");
        settings.scrapex_server_url = j.value("scrapex_server_url", "ws://localhost:9093");
        return settings;
    } catch (const nlohmann::json::parse_error& e) {
        return std::unexpected(ConfigError::JsonParseError);
    } catch (const std::exception& e) {
        return std::unexpected(ConfigError::ReadError);
    }
}

std::expected<void, ConfigError> ConfigManager::save(const Settings& settings) {
    std::ofstream ofs(config_path_);
    if (!ofs) {
        return std::unexpected(ConfigError::WriteError);
    }
    nlohmann::json j = {
        {"user_display_name", settings.user_display_name},
        {"system_prompt", settings.system_prompt},
        {"xai_api_key", settings.xai_api_key},
        {"claude_api_key", settings.claude_api_key},
        {"openai_api_key", settings.openai_api_key},
        {"gemini_api_key", settings.gemini_api_key},
        {"provider", settings.provider},
        {"model", settings.model},
        {"store_chat_history", settings.store_chat_history},
        {"theme_id", settings.theme_id},
        {"mcp_server_url", settings.mcp_server_url},
        {"scrapex_server_url", settings.scrapex_server_url}
    };
    try {
        ofs << j.dump(2);
        return {};
    } catch (const std::exception& e) {
        return std::unexpected(ConfigError::WriteError);
    }
}
