#include "SettingsPanel.hpp"
#include <curses.h>
#include "GlobalLogger.hpp"

void SettingsPanel::draw(WINDOW* win) {
    get_logger().log(LogLevel::Debug, "SettingsPanel::draw(WINDOW*) called");
    int rows, cols;
    getmaxyx(win, rows, cols);
    int win_width = cols;
    int win_height = rows;
    int row = 2;
    werase(win);
    // Removed debug string
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
            case FieldType::Model:
                label = "Model";
                value = in_edit_mode_ && selected_option_ == i ? edit_buffer_ : settings_.model;
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
}
