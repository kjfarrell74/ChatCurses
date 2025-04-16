#include "utf8_utils.hpp"
#include <cwchar>
#include <locale>
#include <codecvt>
#include <vector>
#include <string>
#include <sstream>
#include <wchar.h>
#include <cassert>

// Helper: Convert a UTF-8 string to a vector of Unicode codepoints (wchar_t)
static std::wstring utf8_to_wstring(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    return conv.from_bytes(str);
}

int utf8_display_width(const std::string& s) {
    std::wstring ws = utf8_to_wstring(s);
    int width = 0;
    for (wchar_t wc : ws) {
        int w = wcwidth(wc);
        width += (w > 0) ? w : 0;
    }
    return width;
}

// indent: display width (in spaces) for wrapped lines (not for the first line)
std::vector<std::string> utf8_word_wrap(const std::string& text, int max_width, int indent) {
    std::vector<std::string> lines;
    std::wstring ws = utf8_to_wstring(text);
    std::string line;
    int line_width = 0;
    bool is_first_line = true;
    std::string token;
    int token_width = 0;
    auto flush_token = [&]() {
        if (!token.empty()) {
            // If token doesn't fit, wrap
            if ((line_width > 0 ? line_width + 1 : 0) + token_width > max_width && !line.empty()) {
                lines.push_back(line);
                // All wrapped lines get indent spaces at the start
                line = std::string(indent, ' ') + token;
                line_width = indent + token_width;
                is_first_line = false;
            } else {
                if (!line.empty()) {
                    line += " ";
                    line_width += 1;
                }
                line += token;
                line_width += token_width;
            }
            token.clear();
            token_width = 0;
        }
    };
    size_t i = 0;
    while (i < ws.size()) {
        // Skip spaces (treat as token boundaries)
        if (ws[i] == L' ' || ws[i] == L'\n') {
            flush_token();
            if (ws[i] == L'\n') {
                lines.push_back(line);
                line.clear();
                line_width = 0;
                is_first_line = true;
            }
            ++i;
            continue;
        }
        // Start of a token (word + attached punctuation)
        size_t start = i;
        std::string curr_token;
        int curr_width = 0;
        while (i < ws.size() && ws[i] != L' ' && ws[i] != L'\n') {
            wchar_t wc = ws[i];
            int w = wcwidth(wc);
            if (w < 0) w = 0;
            std::wstring onechar(1, wc);
            std::string utf8char = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(onechar);
            curr_token += utf8char;
            curr_width += w;
            ++i;
        }
        // If token is too long for a line, break it up
        if (curr_width > max_width - (is_first_line ? 0 : indent)) {
    // Hyphenate if possible
    size_t pos = 0;
    int acc_width = 0;
    std::string partial;
    for (size_t j = 0; j < curr_token.size();) {
        wchar_t wc;
        int char_len = 1;
        // Decode one UTF-8 char
        if ((curr_token[j] & 0x80) == 0) { // 1 byte
            wc = curr_token[j];
            char_len = 1;
        } else if ((curr_token[j] & 0xE0) == 0xC0) { // 2 bytes
            wc = ((curr_token[j] & 0x1F) << 6) | (curr_token[j+1] & 0x3F);
            char_len = 2;
        } else if ((curr_token[j] & 0xF0) == 0xE0) { // 3 bytes
            wc = ((curr_token[j] & 0x0F) << 12) | ((curr_token[j+1] & 0x3F) << 6) | (curr_token[j+2] & 0x3F);
            char_len = 3;
        } else { // 4 bytes
            wc = ((curr_token[j] & 0x07) << 18) | ((curr_token[j+1] & 0x3F) << 12) | ((curr_token[j+2] & 0x3F) << 6) | (curr_token[j+3] & 0x3F);
            char_len = 4;
        }
        int w = wcwidth(wc);
        if (w < 0) w = 0;
        if ((is_first_line ? 0 : line_width + 1) + acc_width + w > max_width - 1) { // -1 for hyphen
            // Flush what we have
            if (!partial.empty()) {
                flush_token();
                token = partial + "-";
                token_width = acc_width + 1;
                flush_token();
                partial.clear();
                acc_width = 0;
            }
        }
        partial += curr_token.substr(j, char_len);
        acc_width += w;
        j += char_len;
    }
    if (!partial.empty()) {
        token = partial;
        token_width = acc_width;
        flush_token();
    }


            for (size_t j = 0; j < curr_token.size();) {
                wchar_t wc;
                int char_len = 1;
                // Decode one UTF-8 char
                if ((curr_token[j] & 0x80) == 0) { // 1 byte
                    wc = curr_token[j];
                    char_len = 1;
                } else if ((curr_token[j] & 0xE0) == 0xC0) { // 2 bytes
                    wc = ((curr_token[j] & 0x1F) << 6) | (curr_token[j+1] & 0x3F);
                    char_len = 2;
                } else if ((curr_token[j] & 0xF0) == 0xE0) { // 3 bytes
                    wc = ((curr_token[j] & 0x0F) << 12) | ((curr_token[j+1] & 0x3F) << 6) | (curr_token[j+2] & 0x3F);
                    char_len = 3;
                } else { // 4 bytes
                    wc = ((curr_token[j] & 0x07) << 18) | ((curr_token[j+1] & 0x3F) << 12) | ((curr_token[j+2] & 0x3F) << 6) | (curr_token[j+3] & 0x3F);
                    char_len = 4;
                }
                int w = wcwidth(wc);
                if (w < 0) w = 0;
                if ((is_first_line ? 0 : line_width + 1) + acc_width + w > max_width) {
                    // Flush what we have
                    if (!partial.empty()) {
                        flush_token();
                        token = partial;
                        token_width = acc_width;
                        flush_token();
                        partial.clear();
                        acc_width = 0;
                    }
                }
                partial += curr_token.substr(j, char_len);
                acc_width += w;
                j += char_len;
            }
            if (!partial.empty()) {
                token = partial;
                token_width = acc_width;
                flush_token();
            }
        } else {
            token = curr_token;
            token_width = curr_width;
            flush_token();
        }
    }
    flush_token();
    if (!line.empty()) lines.push_back(line);
    return lines;
}
