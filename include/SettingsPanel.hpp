#pragma once
#include <string>
#include <curses.h>
#include <vector>
#include <map>
#include "ProviderConfig.hpp"

struct Settings {
    std::string user_display_name;
    std::string system_prompt;
    std::string xai_api_key;
    std::string claude_api_key;
    std::string openai_api_key;
    std::string provider = "xai";
    std::string model = "grok-3-beta";
    bool store_chat_history = true;
    int theme_id = 0;

    // Returns the display name for the current provider
    std::string get_display_provider() const {
        return ProviderRegistry::instance().display_name(provider);
    }

    // Returns the appropriate API key for the current provider
    std::string get_api_key() const {
        auto field = ProviderRegistry::instance().api_key_field(provider);
        if (field == "xai_api_key") return xai_api_key;
        if (field == "claude_api_key") return claude_api_key;
        if (field == "openai_api_key") return openai_api_key;
        return {};
    }

    // Sets model to the provider's default if the provider changes
    void initialize_defaults() {
        model = ProviderRegistry::instance().default_model(provider);
    }
};

class ConfigManager;
#include "ConfigManager.hpp"

class SettingsPanel {
public:
    SettingsPanel(Settings& settings, ConfigManager* config_manager = nullptr);
    void draw();
    void draw(WINDOW* win);
    void handle_input(int ch);
    bool is_visible() const;
    void set_visible(bool visible);
    void set_config_manager(ConfigManager* config_manager);
private:
    enum class FieldType { DisplayName, SystemPrompt, XaiApiKey, ClaudeApiKey, OpenaiApiKey, Provider, Model, StoreHistory, Theme, COUNT };
    Settings& settings_;
    ConfigManager* config_manager_ = nullptr;
    bool visible_ = false;
    int selected_option_ = 0;
    bool in_edit_mode_ = false;
    std::string edit_buffer_;
    void draw_option(WINDOW* win, int row, const std::string& label, const std::string& value, bool selected, bool editing);
};
