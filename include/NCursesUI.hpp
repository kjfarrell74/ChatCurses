#pragma once
#include <string>
#include <vector>
#include <memory>
#include <curses.h>
#include <stdexcept> // For std::runtime_error

// RAII wrapper for ncurses WINDOW
class NcursesWindow {
    WINDOW* win_ = nullptr;
public:
    NcursesWindow(WINDOW* win = nullptr) : win_(win) {
        if (!win_) {
            // Allow default construction, but check before use
        }
    }
    ~NcursesWindow() {
        if (win_) {
            delwin(win_);
        }
    }
    // Non-copyable
    NcursesWindow(const NcursesWindow&) = delete;
    NcursesWindow& operator=(const NcursesWindow&) = delete;
    // Movable
    NcursesWindow(NcursesWindow&& other) noexcept : win_(other.win_) { other.win_ = nullptr; }
    NcursesWindow& operator=(NcursesWindow&& other) noexcept {
        if (this != &other) {
            if (win_) { delwin(win_); }
            win_ = other.win_;
            other.win_ = nullptr;
        }
        return *this;
    }

    WINDOW* get() const { return win_; }
    operator WINDOW*() const { return win_; } // Implicit conversion

    void reset(WINDOW* win = nullptr) {
        if (win_) delwin(win_);
        win_ = win;
    }
};

class NCursesUI {
public:
    NCursesUI() noexcept;
    ~NCursesUI() noexcept;
    [[nodiscard]] int draw_chat_window(const std::vector<std::string_view>& messages, int scroll_offset, bool waiting_for_ai = false);
    void draw_input_window(std::string_view input, int cursor_pos);
    void draw_settings_panel(bool visible);
    void refresh_all();
    void toggle_settings_panel();
    void set_theme(int theme_id);
    void handle_resize();
    void show_error(std::string_view message);
    WINDOW* get_settings_win() const { return settings_win_.get(); }
private:
    NcursesWindow chat_win_;
    NcursesWindow input_win_;
    NcursesWindow settings_win_; 
    bool settings_visible_ = false;
    int theme_id_ = 0;
    void init_windows();
    void destroy_windows();
};
