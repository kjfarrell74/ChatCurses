#pragma once
#include <string>
#include <vector>

// Split a UTF-8 string into lines of at most max_width columns (not bytes).
// Returns a vector of lines, each of which fits in max_width display columns.
std::vector<std::string> utf8_word_wrap(const std::string& text, int max_width, int indent = 0);

// Returns the display width (columns) of a UTF-8 string.
int utf8_display_width(const std::string& s);

// Get the next UTF-8 codepoint from a string
uint32_t utf8_next_codepoint(const char* str, size_t& index);

// Get the display width of a single codepoint
int codepoint_width(uint32_t codepoint);