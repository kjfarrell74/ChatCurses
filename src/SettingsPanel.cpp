#include "SettingsPanel.hpp"
#include "ProviderConfig.hpp"
#include <string>
#include <vector>
#include <format>
#include <curses.h>
#include "GlobalLogger.hpp"

// Helper class for RAII window management
class NcursesWindow {
public:
    NcursesWindow(WINDOW* win) : win_(win) {}
    ~NcursesWindow() { if (win_) delwin(win_); }
    WINDOW* get() { return win_; }
private:
    WINDOW* win_;
};

SettingsPanel::SettingsPanel(Settings& settings, ConfigManager* config_manager)
    : settings_(settings), config_manager_(config_manager) {}

void SettingsPanel::set_config_manager(ConfigManager* config_manager) {
    config_manager_ = config_manager;
}

void SettingsPanel::draw() {
    // Call the draw(WINDOW*) implementation using a temporary window
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    int win_width = std::max(40, cols * 2 / 3);
    int win_height = std::min(rows - 2, 14);
    int startx = (cols - win_width) / 2;
    int starty = (rows - win_height) / 2;
    
    NcursesWindow temp_win(newwin(win_height, win_width, starty, startx));
    if (temp_win.get()) {
        keypad(temp_win.get(), TRUE);
        draw(temp_win.get());
    } else {
        get_logger().log(LogLevel::Error, "Failed to create settings window");
    }
}

void SettingsPanel::draw(WINDOW* win) {
    get_logger().log(LogLevel::Debug, "SettingsPanel::draw(WINDOW*) called");
    int rows, cols;
    getmaxyx(win, rows, cols);
    int win_height = rows;
    int row = 2;
    werase(win);
    
    for (int i = 0; i < (int)FieldType::COUNT; ++i) {
        // Show only the relevant API key field for the selected provider
        auto api_key_field = ProviderRegistry::instance().api_key_field(settings_.provider);
        if ((FieldType)i == FieldType::XaiApiKey && api_key_field != "xai_api_key") continue;
        if ((FieldType)i == FieldType::ClaudeApiKey && api_key_field != "claude_api_key") continue;
        if ((FieldType)i == FieldType::OpenaiApiKey && api_key_field != "openai_api_key") continue;
        if ((FieldType)i == FieldType::GeminiApiKey && api_key_field != "gemini_api_key") continue;
        if ((FieldType)i == FieldType::MCPServerUrl && settings_.provider != "mcp") continue;

        std::string label, value;
        bool editing = in_edit_mode_ && selected_option_ == i;
        
        switch ((FieldType)i) {
            case FieldType::DisplayName:
                label = "Display Name";
                value = in_edit_mode_ && selected_option_ == i ? edit_buffer_ : settings_.user_display_name;
                break;
            case FieldType::SystemPrompt:
                label = "System Prompt";
                value = in_edit_mode_ && selected_option_ == i ? edit_buffer_ : settings_.system_prompt;
                break;
            case FieldType::XaiApiKey:
                label = "xAI API Key";
                value = (in_edit_mode_ && selected_option_ == i) ? edit_buffer_ : (settings_.xai_api_key.empty() ? "<not set>" : "<hidden>");
                break;
            case FieldType::ClaudeApiKey:
                label = "Claude API Key";
                value = (in_edit_mode_ && selected_option_ == i) ? edit_buffer_ : (settings_.claude_api_key.empty() ? "<not set>" : "<hidden>");
                break;
            case FieldType::OpenaiApiKey:
                label = "OpenAI API Key";
                value = (in_edit_mode_ && selected_option_ == i) ? edit_buffer_ : (settings_.openai_api_key.empty() ? "<not set>" : "<hidden>");
                break;
            case FieldType::GeminiApiKey:
                label = "Gemini API Key";
                value = (in_edit_mode_ && selected_option_ == i) ? edit_buffer_ : (settings_.gemini_api_key.empty() ? "<not set>" : "<hidden>");
                break;
            case FieldType::Provider:
                label = "Provider";
                value = settings_.get_display_provider();
                break;
            case FieldType::Model:
                {
                    label = "Model";
                    // If in edit mode, show the edit buffer
                    if (in_edit_mode_ && selected_option_ == i) {
                        value = edit_buffer_;
                    } else {
                        // Get the list of available models for the current provider
                        auto& registry = ProviderRegistry::instance();
                        auto available_models = registry.models(settings_.provider);
                        
                        // Find current model in the list
                        auto it = std::find(available_models.begin(), available_models.end(), settings_.model);
                        if (it != available_models.end()) {
                            size_t index = std::distance(available_models.begin(), it) + 1;
                            value = std::format("{}/{}: {}", index, available_models.size(), settings_.model);
                        } else {
                            value = settings_.model;
                        }
                    }
                }
                break;
            case FieldType::StoreHistory:
                label = "Store Chat History";
                value = settings_.store_chat_history ? "Yes" : "No";
                break;
            case FieldType::Theme:
                label = "Theme";
                value = std::to_string(settings_.theme_id);
                break;
            case FieldType::MCPServerUrl:
                label = "MCP Server URL";
                value = in_edit_mode_ && selected_option_ == i ? edit_buffer_ : settings_.mcp_server_url;
                break;
            case FieldType::ScrapexServerUrl:
                label = "ScrapeX Server URL";
                value = in_edit_mode_ && selected_option_ == i ? edit_buffer_ : settings_.scrapex_server_url;
                break;
            default: break;
        }
        draw_option(win, row++, label, value, selected_option_ == i, editing);
    }
    
    box(win, 0, 0);
    mvwprintw(win, win_height - 5, 2, "Use Up/Down to navigate, Enter to edit, ESC to exit");
    mvwprintw(win, win_height - 4, 2, "Use Left/Right arrows to cycle Provider and Model options");
    if (in_edit_mode_) {
        mvwprintw(win, win_height - 3, 2, "Type to edit, Enter to save, ESC to cancel");
    }
    wrefresh(win);
}

void SettingsPanel::handle_input(int ch) {
    if (in_edit_mode_) {
        if (ch == 27) { // ESC
            in_edit_mode_ = false;
            edit_buffer_.clear();
            return;
        } else if (ch == 10 || ch == KEY_ENTER) {
            // Save edit
            switch ((FieldType)selected_option_) {
                case FieldType::DisplayName:
                    settings_.user_display_name = edit_buffer_;
                    break;
                case FieldType::SystemPrompt:
                    settings_.system_prompt = edit_buffer_;
                    break;
                case FieldType::XaiApiKey:
                    settings_.xai_api_key = edit_buffer_;
                    break;
                case FieldType::ClaudeApiKey:
                    settings_.claude_api_key = edit_buffer_;
                    break;
                case FieldType::OpenaiApiKey:
                    settings_.openai_api_key = edit_buffer_;
                    break;
                case FieldType::GeminiApiKey:
                    settings_.gemini_api_key = edit_buffer_;
                    break;
                case FieldType::Provider:
                    // Cycle provider
                    if (settings_.provider == "xai") {
                        settings_.provider = "claude";
                        settings_.model = "claude";
                    } else if (settings_.provider == "claude") {
                        settings_.provider = "openai";
                        settings_.model = "gpt-4o";
                    } else {
                        settings_.provider = "xai";
                        settings_.model = "grok-3-beta";
                    }
                    break;
                case FieldType::Model:
                    settings_.model = edit_buffer_;
                    break;
                case FieldType::MCPServerUrl:
                    settings_.mcp_server_url = edit_buffer_;
                    break;
                case FieldType::ScrapexServerUrl:
                    settings_.scrapex_server_url = edit_buffer_;
                    break;
                default:
                    break;
            }
            if (config_manager_) {
                auto result = config_manager_->save(settings_);
                if (!result) {
                    get_logger().log(LogLevel::Error, "Failed to save settings");
                }
            }
            in_edit_mode_ = false;
            edit_buffer_.clear();
            return;
        } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
            if (!edit_buffer_.empty()) edit_buffer_.pop_back();
        } else if (ch >= 32 && ch <= 126) {
            edit_buffer_ += static_cast<char>(ch);
        }
        return;
    }
    switch (ch) {
        case KEY_UP:
            selected_option_ = (selected_option_ - 1 + (int)FieldType::COUNT) % (int)FieldType::COUNT;
            break;
        case KEY_DOWN:
            selected_option_ = (selected_option_ + 1) % (int)FieldType::COUNT;
            break;
        case KEY_LEFT:
        case KEY_RIGHT:
            if (selected_option_ == (int)FieldType::Provider) {
                // Cycle providers using ProviderRegistry
                auto& registry = ProviderRegistry::instance();
                auto providers = registry.provider_ids();
                auto it = std::find(providers.begin(), providers.end(), settings_.provider);
                if (it != providers.end()) {
                    if (ch == KEY_RIGHT) {
                        ++it;
                        if (it == providers.end()) it = providers.begin();
                    } else {
                        if (it == providers.begin()) it = providers.end();
                        --it;
                    }
                    settings_.provider = *it;
                    settings_.initialize_defaults(); // Set model to default for new provider
                }
            } 
            else if (selected_option_ == (int)FieldType::Model) {
                // Cycle models for current provider using ProviderRegistry
                auto& registry = ProviderRegistry::instance();
                auto available_models = registry.models(settings_.provider);
                auto it = std::find(available_models.begin(), available_models.end(), settings_.model);
                
                if (it != available_models.end()) {
                    if (ch == KEY_RIGHT) {
                        ++it;
                        if (it == available_models.end()) it = available_models.begin();
                    } else {
                        if (it == available_models.begin()) it = available_models.end();
                        --it;
                    }
                    settings_.model = *it;
                    
                    // Save settings when model changes
                    if (config_manager_) {
                        auto result = config_manager_->save(settings_);
                        if (!result) {
                            get_logger().log(LogLevel::Error, "Failed to save settings after model change");
                        }
                    }
                }
            }
            break;
        case 10: // Enter
        case KEY_ENTER:
            if (selected_option_ == (int)FieldType::StoreHistory) {
                settings_.store_chat_history = !settings_.store_chat_history;
                if (config_manager_) {
                    auto result = config_manager_->save(settings_);
                    if (!result) {
                        get_logger().log(LogLevel::Error, "Failed to save settings");
                    }
                }
            } else if (selected_option_ == (int)FieldType::Theme) {
                settings_.theme_id = (settings_.theme_id + 1) % 4; // Example: 4 themes
                if (config_manager_) {
                    auto result = config_manager_->save(settings_);
                    if (!result) {
                        get_logger().log(LogLevel::Error, "Failed to save settings");
                    }
                }
            } else {
                in_edit_mode_ = true;
                switch ((FieldType)selected_option_) {
                    case FieldType::DisplayName:
                        edit_buffer_ = settings_.user_display_name;
                        break;
                    case FieldType::SystemPrompt:
                        edit_buffer_ = settings_.system_prompt;
                        break;
                    case FieldType::XaiApiKey:
                        edit_buffer_ = settings_.xai_api_key;
                        break;
                    case FieldType::ClaudeApiKey:
                        edit_buffer_ = settings_.claude_api_key;
                        break;
                    case FieldType::OpenaiApiKey:
                        edit_buffer_ = settings_.openai_api_key;
                        break;
                    case FieldType::GeminiApiKey:
                        edit_buffer_ = settings_.gemini_api_key;
                        break;
                    case FieldType::Model:
                        edit_buffer_ = settings_.model;
                        break;
                    case FieldType::MCPServerUrl:
                        edit_buffer_ = settings_.mcp_server_url;
                        break;
                    case FieldType::ScrapexServerUrl:
                        edit_buffer_ = settings_.scrapex_server_url;
                        break;
                    default:
                        break;
                }
            }
            break;
        case 27: // ESC
            set_visible(false);
            break;
        default:
            break;
    }
}

bool SettingsPanel::is_visible() const {
    return visible_;
}

void SettingsPanel::set_visible(bool visible) {
    get_logger().log(LogLevel::Debug, std::string("SettingsPanel::set_visible called with visible = ") + (visible ? "true" : "false"));
    visible_ = visible;
}

void SettingsPanel::draw_option(WINDOW* win, int row, const std::string& label, const std::string& value, bool selected, bool editing)
{
    int attr = A_NORMAL;
    if (selected) attr |= A_REVERSE;
    if (editing) attr |= A_BOLD;
    wmove(win, row, 2);
    wclrtoeol(win);
    wattron(win, attr);
    
    bool show_arrows = false;
    if ((label == "Provider" || label == "Model") && selected && !editing) {
        // Show arrow indicators for options that can be cycled with left/right keys
        mvwprintw(win, row, 2, " %s: « %s » ", label.c_str(), value.c_str());
        show_arrows = true;
    }
    
    if (!show_arrows) {
        mvwprintw(win, row, 2, " %s: %s ", label.c_str(), value.c_str()); // Add padding
    }
    
    wattroff(win, attr);
}