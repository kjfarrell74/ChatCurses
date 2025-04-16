#include "utf8_utils.hpp"
#include "utf8cpp/utf8.h"
#include <cwchar>
#include <limits>
#include <locale>
#include <vector>
#include <string>
#include <sstream>

// Get the next UTF-8 codepoint from a string
uint32_t utf8_next_codepoint(const char* str, size_t& index) {
    const char* start = str + index;
    const char* end = start;
    while (*end) end++;
    
    if (start >= end)
        return 0;
    
    uint32_t codepoint;
    try {
        // Get the codepoint
        codepoint = utf8::next(start, end);
        // Update index to point after the current character
        index = start - str;
    } catch (const utf8::exception& e) {
        // Handle invalid UTF-8
        index++;
        return 0xFFFD; // Replacement character
    }
    
    return codepoint;
}

// Get the display width of a single codepoint using wcwidth
int codepoint_width(uint32_t codepoint) {
    // wcwidth works for values that fit in wchar_t
    if (codepoint <= static_cast<uint32_t>(std::numeric_limits<wchar_t>::max())) {
        int width = wcwidth(static_cast<wchar_t>(codepoint));
        return width >= 0 ? width : 0;
    }
    
    // For codepoints outside wchar_t range, assume width 1
    return 1;
}

int utf8_display_width(const std::string& s) {
    int width = 0;
    
    try {
        const char* start = s.c_str();
        const char* end = start + s.length();
        
        while (start < end) {
            uint32_t codepoint = utf8::next(start, end);
            width += codepoint_width(codepoint);
        }
    } catch (const utf8::exception& e) {
        // On invalid UTF-8, return best-effort width
    }
    
    return width;
}

// indent: display width (in spaces) for wrapped lines (not for the first line)
std::vector<std::string> utf8_word_wrap(const std::string& text, int max_width, int indent) {
    std::vector<std::string> lines;
    if (text.empty() || max_width <= 0)
        return lines;
    
    std::string line;
    int line_width = 0;
    std::string word;
    int word_width = 0;
    bool is_first_line = true;
    
    auto add_word = [&]() {
        if (word.empty())
            return;
            
        // Check if adding this word would exceed max_width
        int needed_width = line_width + (line.empty() ? 0 : 1) + word_width;
        
        if (needed_width > max_width && !line.empty()) {
            // Add current line to lines and start a new one
            lines.push_back(line);
            line = std::string(indent, ' ');
            line_width = indent;
            is_first_line = false;
        }
        
        if (!line.empty() && line[line.length() - 1] != ' ') {
            line += ' ';
            line_width += 1;
        }
        
        // Add the word to the current line
        line += word;
        line_width += word_width;
        
        // Reset word
        word.clear();
        word_width = 0;
    };
    
    try {
        const char* start = text.c_str();
        const char* end = start + text.length();
        const char* token_start = start;
        
        while (start < end) {
            // Save the current position
            const char* char_start = start;
            
            // Get the next character and advance the iterator
            uint32_t codepoint = utf8::next(start, end);
            
            // Get the UTF-8 character string representation
            std::string utf8_char(char_start, start);
            
            if (codepoint == ' ' || codepoint == '\n' || codepoint == '\t') {
                // End of word
                add_word();
                
                if (codepoint == '\n') {
                    // End of line
                    lines.push_back(line);
                    line.clear();
                    line_width = 0;
                    is_first_line = true;
                }
            } else {
                // Part of a word
                word += utf8_char;
                word_width += codepoint_width(codepoint);
                
                // Handle exceptionally long words
                if (word_width > max_width - (is_first_line ? 0 : indent)) {
                    // Add as much of the word as we can fit
                    if (!line.empty()) {
                        lines.push_back(line);
                        line.clear();
                        line_width = 0;
                        is_first_line = false;
                    }
                    
                    // If word fits on a line by itself, add it
                    if (word_width <= max_width) {
                        line = is_first_line ? "" : std::string(indent, ' ');
                        line_width = is_first_line ? 0 : indent;
                        line += word;
                        line_width += word_width;
                        
                        lines.push_back(line);
                        line.clear();
                        line_width = 0;
                        is_first_line = false;
                    } else {
                        // The word needs to be broken up - character by character with hyphens
                        std::string part;
                        int part_width = 0;
                        
                        const char* word_ptr = word.c_str();
                        const char* word_end = word_ptr + word.length();
                        
                        while (word_ptr < word_end) {
                            const char* char_pos = word_ptr;
                            uint32_t cp = utf8::next(word_ptr, word_end);
                            int cp_width = codepoint_width(cp);
                            
                            if (part_width + cp_width > max_width - 1) { // -1 for hyphen
                                part += "-";
                                if (is_first_line) {
                                    lines.push_back(part);
                                } else {
                                    lines.push_back(std::string(indent, ' ') + part);
                                }
                                part.clear();
                                part_width = 0;
                                is_first_line = false;
                            }
                            
                            part.append(char_pos, word_ptr);
                            part_width += cp_width;
                        }
                        
                        if (!part.empty()) {
                            line = is_first_line ? "" : std::string(indent, ' ');
                            line_width = is_first_line ? 0 : indent;
                            line += part;
                            line_width += part_width;
                        }
                    }
                    
                    word.clear();
                    word_width = 0;
                }
            }
        }
        
        // Add any remaining word
        add_word();
        
        // Add the last line if not empty
        if (!line.empty()) {
            lines.push_back(line);
        }
    } catch (const utf8::exception& e) {
        // On invalid UTF-8, add what we have so far
        add_word();
        if (!line.empty()) {
            lines.push_back(line);
        }
    }
    
    return lines;
}