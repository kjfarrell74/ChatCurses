#include "ConfigManager.hpp"
#include <fstream>
#include <nlohmann/json.hpp>

ConfigManager::ConfigManager(const std::string& config_path) : config_path_(config_path) {}

bool ConfigManager::load(Settings& settings) {
    std::ifstream ifs(config_path_);
    if (!ifs) return false;
    nlohmann::json j;
    ifs >> j;
    settings.user_display_name = j.value("user_display_name", "User");
    settings.system_prompt = j.value("system_prompt", "");
    settings.api_key = j.value("api_key", "");
    settings.ai_model = j.value("ai_model", "");
    settings.store_chat_history = j.value("store_chat_history", true);
    settings.theme_id = j.value("theme_id", 0);
    return true;
}

bool ConfigManager::save(const Settings& settings) {
    std::ofstream ofs(config_path_);
    if (!ofs) return false;
    nlohmann::json j = {
        {"user_display_name", settings.user_display_name},
        {"system_prompt", settings.system_prompt},
        {"api_key", settings.api_key},
        {"ai_model", settings.ai_model},
        {"store_chat_history", settings.store_chat_history},
        {"theme_id", settings.theme_id}
    };
    ofs << j.dump(2);
    return true;
}
