#include "CommandLineEditor.hpp"
#include <curses.h>

CommandLineEditor::CommandLineEditor() {}

void CommandLineEditor::handle_input(int ch) {
    // Left/Right navigation, Home/End, Delete, Backspace, Insert, Printable
    switch (ch) {
        case KEY_LEFT:
            if (cursor_pos_ > 0) --cursor_pos_;
            break;
        case KEY_RIGHT:
            if (cursor_pos_ < (int)buffer_.size()) ++cursor_pos_;
            break;
        case KEY_HOME:
            cursor_pos_ = 0;
            break;
        case KEY_END:
            cursor_pos_ = buffer_.size();
            break;
        case KEY_DC: // Delete key
            if (cursor_pos_ < (int)buffer_.size()) buffer_.erase(cursor_pos_, 1);
            break;
        case 127: case KEY_BACKSPACE: case 8: // Backspace
            if (cursor_pos_ > 0 && !buffer_.empty()) {
                buffer_.erase(cursor_pos_-1, 1);
                --cursor_pos_;
            }
            break;
        default:
            if (ch >= 32 && ch <= 126) { // Printable ASCII
                buffer_.insert(cursor_pos_, 1, static_cast<char>(ch));
                ++cursor_pos_;
            }
            break;
    }
    // Clamp cursor position
    if (cursor_pos_ > (int)buffer_.size()) cursor_pos_ = buffer_.size();
    if (cursor_pos_ < 0) cursor_pos_ = 0;
}

std::string CommandLineEditor::current_line() const {
    return buffer_;
}

void CommandLineEditor::clear() {
    buffer_.clear();
    cursor_pos_ = 0;
    history_index_ = -1;
}

void CommandLineEditor::add_history(const std::string& line) {
    if (!line.empty()) {
        history_.push_back(line);
        history_index_ = history_.size();
    }
}

std::string CommandLineEditor::history_up() {
    if (history_.empty() || history_index_ <= 0) return buffer_;
    --history_index_;
    buffer_ = history_[history_index_];
    return buffer_;
}

std::string CommandLineEditor::history_down() {
    if (history_.empty() || history_index_ >= (int)history_.size() - 1) return buffer_;
    ++history_index_;
    buffer_ = history_[history_index_];
    return buffer_;
}

const std::vector<std::string>& CommandLineEditor::history() const {
    return history_;
}
