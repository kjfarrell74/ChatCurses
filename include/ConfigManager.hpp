#pragma once
#include <string>
#include <expected> // C++23
#include <system_error> // For error codes
#include "SettingsPanel.hpp"

enum class ConfigError {
    FileNotFound,
    ReadError,
    WriteError,
    JsonParseError,
    Unknown
};

class ConfigManager {
public:
    ConfigManager(const std::string& config_path = "chatbot_config.json");
    // Returns loaded settings on success, or an error code on failure
    std::expected<Settings, ConfigError> load();
    // Returns nothing on success, or an error code on failure
    std::expected<void, ConfigError> save(const Settings& settings);
public:
    const std::string& config_path() const { return config_path_; }
private:
    std::string config_path_;
};
