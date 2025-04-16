#include "SettingsPanel.hpp"
#include <string>
#include <vector>
#include <format>
#include <curses.h>
#include "Logger.hpp"

SettingsPanel::SettingsPanel(Settings& settings, ConfigManager* config_manager)
    : settings_(settings), config_manager_(config_manager) {}

void SettingsPanel::set_config_manager(ConfigManager* config_manager) {
    config_manager_ = config_manager;
}

void SettingsPanel::draw() {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    int win_width = std::max(40, cols * 2 / 3);
    int win_height = std::min(rows - 2, 14);
    int startx = (cols - win_width) / 2;
    int starty = (rows - win_height) / 2;
    WINDOW* win = newwin(win_height, win_width, starty, startx);
    keypad(win, TRUE);
    int row = 2;
    for (int i = 0; i < (int)FieldType::COUNT; ++i) {
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
            case FieldType::ApiKey:
                label = "API Key";
                value = (in_edit_mode_ && selected_option_ == i) ? edit_buffer_ : (settings_.api_key.empty() ? "<not set>" : "<hidden>");
                break;
            case FieldType::Model:
                label = "Model";
                value = in_edit_mode_ && selected_option_ == i ? edit_buffer_ : settings_.ai_model;
                break;
            case FieldType::StoreHistory:
                label = "Store Chat History";
                value = settings_.store_chat_history ? "Yes" : "No";
                break;
            case FieldType::Theme:
                label = "Theme";
                value = std::to_string(settings_.theme_id);
                break;
            default: break;
        }
        draw_option(win, row++, label, value, selected_option_ == i, editing);
    }
    box(win, 0, 0);
    mvwprintw(win, win_height - 4, 2, "Use Arrow keys to navigate, Enter to edit, ESC to exit");
    if (in_edit_mode_) {
        mvwprintw(win, win_height - 3, 2, "Type to edit, Enter to save, ESC to cancel");
    }
    wrefresh(win);
    delwin(win);
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
                case FieldType::ApiKey:
                    settings_.api_key = edit_buffer_;
                    break;
                case FieldType::Model:
                    settings_.ai_model = edit_buffer_;
                    break;
                default:
                    break;
            }
            if (config_manager_) {
                auto result = config_manager_->save(settings_);
                if (!result) { /* Log or display error */ }
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
        case 10: // Enter
        case KEY_ENTER:
            if (selected_option_ == (int)FieldType::StoreHistory) {
                settings_.store_chat_history = !settings_.store_chat_history;
                if (config_manager_) {
                    auto result = config_manager_->save(settings_);
                    if (!result) { /* Log or display error */ }
                }
            } else if (selected_option_ == (int)FieldType::Theme) {
                settings_.theme_id = (settings_.theme_id + 1) % 4; // Example: 4 themes
                if (config_manager_) {
                    auto result = config_manager_->save(settings_);
                    if (!result) { /* Log or display error */ }
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
                    case FieldType::ApiKey:
                        edit_buffer_ = settings_.api_key;
                        break;
                    case FieldType::Model:
                        edit_buffer_ = settings_.ai_model;
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
    static Logger logger_("chatbot.log");
    logger_.log(Logger::Level::Debug, std::string("SettingsPanel::set_visible called with visible = ") + (visible ? "true" : "false"));
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
    mvwprintw(win, row, 2, " %s: %s ", label.c_str(), value.c_str()); // Add padding
    wattroff(win, attr);
}
