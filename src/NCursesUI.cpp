#include "NCursesUI.hpp"
#include <stdexcept>
#include "utf8_utils.hpp"

NCursesUI::NCursesUI() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(1);
    start_color();
    init_windows();
    ::refresh(); // Force initial paint
}

NCursesUI::~NCursesUI() {
    // Windows are automatically deleted by NcursesWindow destructors
    endwin();
}

void NCursesUI::init_windows() {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    int input_height = 3;
    chat_win_.reset(newwin(rows - input_height, cols, 0, 0));
    input_win_.reset(newwin(input_height, cols, rows - input_height, 0));
    settings_win_.reset(newwin(rows, cols, 0, 0));
    if (!chat_win_ || !input_win_ || !settings_win_) {
        endwin();
        throw std::runtime_error("Failed to create ncurses windows");
    }
}


int NCursesUI::draw_chat_window(const std::vector<std::string>& messages, int scroll_offset, bool waiting_for_ai) {
    werase(chat_win_.get());
    int maxy, maxx;
    getmaxyx(chat_win_.get(), maxy, maxx);
    std::vector<std::string> wrapped_lines;
    std::vector<int> is_first_line;
    
    for (const auto& msg : messages) {
        bool is_user_msg = msg.starts_with("User: ");
        bool is_ai_msg = msg.starts_with("AI: ");
        std::string prefix, content;
        int prefix_width;
        if (is_user_msg) {
            prefix = "Kieran: ";
            content = msg.substr(prefix.length());
        } else if (is_ai_msg) {
            prefix = "AI: ";
            content = msg.substr(prefix.length());
        } else {
            prefix = "";
            content = msg;
        }
        prefix_width = utf8_display_width(prefix);
        int available_width = maxx - 2 - prefix_width;
        std::string indent(prefix_width, ' ');
        auto lines = utf8_word_wrap(content, available_width, prefix_width);
        if (!lines.empty()) {
            wrapped_lines.push_back(prefix + lines[0]);
            is_first_line.push_back(1);
            for (size_t i = 1; i < lines.size(); ++i) {
                wrapped_lines.push_back(lines[i]);
                is_first_line.push_back(0);
            }
        }
        
        // Add a blank line after each message
        wrapped_lines.push_back("");
        is_first_line.push_back(2); // 2 means blank line
    }
    // Scroll_offset now applies to wrapped lines, not messages
    int total_lines = wrapped_lines.size();
    int display_lines = maxy - 2;
    int start_line = std::max(0, total_lines - display_lines - scroll_offset);
    int end_line = std::min(start_line + display_lines, total_lines);
    int line = 1;
    for (int i = start_line; i < end_line; ++i, ++line) {
        if (is_first_line[i] == 2) { continue; } // skip printing for blank line, just increment line
        
        // Ensure text is printed exactly at column 1 for consistent alignment
        // This ensures the indentation we created actually appears properly
        mvwprintw(chat_win_, line, 1, "%s", wrapped_lines[i].c_str());
    }
    if (waiting_for_ai && line < maxy - 1) {
        mvwprintw(chat_win_, maxy - 2, 2, "[Waiting for AI response...]");
    }
    box(chat_win_, 0, 0);
    wrefresh(chat_win_);
    return total_lines;
}

void NCursesUI::draw_input_window(const std::string& input, int cursor_pos) {
    werase(input_win_);
    box(input_win_, 0, 0);
    mvwprintw(input_win_, 1, 1, "%s", input.c_str());
    // Move the cursor to the logical position
    wmove(input_win_, 1, 1 + cursor_pos);
    wrefresh(input_win_);
}

void NCursesUI::draw_settings_panel(bool visible) {
    settings_visible_ = visible;
    if (!visible) { return; }
    werase(settings_win_);
    box(settings_win_, 0, 0);
    mvwprintw(settings_win_, 1, 2, "Settings Panel (stub)");
    wrefresh(settings_win_);
}

void NCursesUI::refresh_all() {
    wrefresh(chat_win_);
    wrefresh(input_win_);
    if (settings_visible_) { wrefresh(settings_win_); }
}

void NCursesUI::toggle_settings_panel() {
    settings_visible_ = !settings_visible_;
    draw_settings_panel(settings_visible_);
}

void NCursesUI::set_theme(int theme_id) {
    theme_id_ = theme_id;
    // TODO: implement color themes
}

void NCursesUI::handle_resize() {
    destroy_windows();
    init_windows();
    // After resizing, force redraw of all windows
    refresh_all();
    // Optionally, you may want to trigger a redraw of the chat window with the latest state.
    // This requires access to the latest messages, scroll_offset, and waiting_for_ai status.
    // If those are available as member variables, call draw_chat_window here. Otherwise, ensure the main loop triggers a redraw after resize.
}

void NCursesUI::show_error(const std::string& message) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    mvprintw(rows - 2, 2, "Error: %s", message.c_str());
    refresh();
}

void NCursesUI::destroy_windows() {
    // Cleanup all ncurses windows
    chat_win_.reset();
    input_win_.reset();
    settings_win_.reset();
}
