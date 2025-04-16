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
        settings.api_key = j.value("api_key", "");
        settings.ai_model = j.value("ai_model", "");
        settings.store_chat_history = j.value("store_chat_history", true);
        settings.theme_id = j.value("theme_id", 0);
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
        {"api_key", settings.api_key},
        {"ai_model", settings.ai_model},
        {"store_chat_history", settings.store_chat_history},
        {"theme_id", settings.theme_id}
    };
    try {
        ofs << j.dump(2);
        return {};
    } catch (const std::exception& e) {
        return std::unexpected(ConfigError::WriteError);
    }
}
