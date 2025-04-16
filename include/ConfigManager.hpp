#pragma once
#include <string>
#include "SettingsPanel.hpp"

class ConfigManager {
public:
    ConfigManager(const std::string& config_path = "chatbot_config.json");
    bool load(Settings& settings);
    bool save(const Settings& settings);
private:
    std::string config_path_;
};
