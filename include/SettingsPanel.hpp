#pragma once
#include <string>
#include <curses.h>
#include <vector>
#include <map>

struct Settings {
    std::string user_display_name;
    std::string system_prompt;
    std::string api_key;
    std::string ai_model;
    bool store_chat_history = true;
    int theme_id = 0;
};

class ConfigManager;
#include "ConfigManager.hpp"

class SettingsPanel {
public:
    SettingsPanel(Settings& settings, ConfigManager* config_manager = nullptr);
    void draw();
    void handle_input(int ch);
    bool is_visible() const;
    void set_visible(bool visible);
    void set_config_manager(ConfigManager* config_manager);
private:
    enum class FieldType { DisplayName, SystemPrompt, ApiKey, Model, StoreHistory, Theme, COUNT };
    Settings& settings_;
    ConfigManager* config_manager_ = nullptr;
    bool visible_ = false;
    int selected_option_ = 0;
    bool in_edit_mode_ = false;
    std::string edit_buffer_;
    void draw_option(WINDOW* win, int row, const std::string& label, const std::string& value, bool selected, bool editing);
    void save_settings();
};
