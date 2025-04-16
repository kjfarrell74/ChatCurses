#pragma once
#include <string>
#include <vector>
#include <memory>
#include <curses.h>

class NCursesUI {
public:
    NCursesUI();
    ~NCursesUI();
    int draw_chat_window(const std::vector<std::string>& messages, int scroll_offset, bool waiting_for_ai = false);
    void draw_input_window(const std::string& input, int cursor_pos);
    void draw_settings_panel(bool visible);
    void refresh_all();
    void toggle_settings_panel();
    void set_theme(int theme_id);
    void handle_resize();
    void show_error(const std::string& message);
    // ... more as needed
private:
    WINDOW* chat_win_;
    WINDOW* input_win_;
    WINDOW* settings_win_;
    bool settings_visible_ = false;
    int theme_id_ = 0;
    void init_windows();
    void destroy_windows();
};
