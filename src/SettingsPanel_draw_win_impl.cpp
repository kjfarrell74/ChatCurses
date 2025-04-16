#include "SettingsPanel.hpp"
#include <curses.h>
#include "Logger.hpp"

void SettingsPanel::draw(WINDOW* win) {
    static Logger logger_("chatbot.log");
    logger_.log(Logger::Level::Debug, "SettingsPanel::draw(WINDOW*) called");
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
}
