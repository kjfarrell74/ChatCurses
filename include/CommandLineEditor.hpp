#pragma once
#include <string>
#include <vector>

class CommandLineEditor {
public:
    CommandLineEditor();
    void handle_input(int ch);
    std::string current_line() const;
    void clear();
    void add_history(const std::string& line);
    std::string history_up();
    std::string history_down();
    const std::vector<std::string>& history() const;
public:
    // New for richer editing
    int cursor_pos() const { return cursor_pos_; }
    void set_cursor_pos(int pos) { cursor_pos_ = std::max(0, std::min((int)buffer_.size(), pos)); }
private:
    std::string buffer_;
    std::vector<std::string> history_;
    int history_index_ = -1;
    int cursor_pos_ = 0;
};
