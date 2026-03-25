#pragma once
#include "esphome.h"
#include <deque>
#include <vector>
#include <string>
#include <algorithm>

namespace watch_assistant {

// Use a class to maintain state across display update cycles
class WatchRenderer {
 public:
    // State variables
    std::deque<std::string> lines;
    std::string text_buffer;
    std::string line_buffer;
    size_t current_char_index = 0;
    size_t current_token_index = 0;
    size_t last_space_index = std::string::npos;
    std::vector<std::string> tokens;
    
    enum ProcessingMode { CHAR_MODE, WORD_MODE, FULL_MODE };
    ProcessingMode last_process_mode = FULL_MODE;

    void render(esphome::display::DisplayBuffer &it, 
                std::string response, 
                std::string selected_mode_str,
                esphome::display::Font *font,
                esphome::Color color_text,
                bool button_pressed) {
        
        const int max_chars_per_line = 26;
        const int max_lines = 8;

        // Map string to Enum
        ProcessingMode process_mode = FULL_MODE;
        if (selected_mode_str == "Character Mode") process_mode = CHAR_MODE;
        else if (selected_mode_str == "Word Mode") process_mode = WORD_MODE;

        bool mode_changed = (process_mode != last_process_mode);

        // Reset logic if content or mode changes
        if (text_buffer != response || mode_changed || button_pressed) {
            lines.clear();
            text_buffer = response;
            line_buffer.clear();
            last_space_index = std::string::npos;
            current_char_index = 0;
            current_token_index = 0;

            if (process_mode == WORD_MODE || process_mode == FULL_MODE) {
                tokens.clear();
                size_t pos = 0;
                while (pos < text_buffer.length()) {
                    char c = text_buffer[pos];
                    if (c == '\n') { tokens.push_back("\n"); pos++; }
                    else if (isspace(c)) {
                        size_t start = pos;
                        while (pos < text_buffer.length() && isspace(text_buffer[pos]) && text_buffer[pos] != '\n') pos++;
                        tokens.push_back(text_buffer.substr(start, pos - start));
                    } else {
                        size_t start = pos;
                        while (pos < text_buffer.length() && !isspace(text_buffer[pos])) pos++;
                        tokens.push_back(text_buffer.substr(start, pos - start));
                    }
                }
            }

            if (process_mode == FULL_MODE) {
                // Pre-calculate all lines for Full Mode
                for (const auto& token : tokens) {
                    if (token == "\n") {
                        if (lines.size() >= max_lines) lines.pop_front();
                        lines.push_back(line_buffer);
                        line_buffer.clear();
                        continue;
                    }
                    if (line_buffer.length() + token.length() <= max_chars_per_line) {
                        line_buffer += token;
                    } else {
                        if (lines.size() >= max_lines) lines.pop_front();
                        lines.push_back(line_buffer);
                        line_buffer = token;
                    }
                }
                if (!line_buffer.empty()) {
                    if (lines.size() >= max_lines) lines.pop_front();
                    lines.push_back(line_buffer);
                    line_buffer.clear();
                }
            }
            last_process_mode = process_mode;
        }

        // Incremental Processing for Char/Word modes
        if (process_mode == CHAR_MODE && current_char_index < text_buffer.length()) {
            char next_char = text_buffer[current_char_index++];
            if (next_char == '\n') {
                if (lines.size() >= max_lines) lines.pop_front();
                lines.push_back(line_buffer);
                line_buffer.clear();
            } else {
                line_buffer += next_char;
                if (line_buffer.length() >= max_chars_per_line) {
                    if (lines.size() >= max_lines) lines.pop_front();
                    lines.push_back(line_buffer);
                    line_buffer.clear();
                }
            }
        } else if (process_mode == WORD_MODE && current_token_index < tokens.size()) {
            std::string token = tokens[current_token_index++];
            if (token == "\n") {
                if (lines.size() >= max_lines) lines.pop_front();
                lines.push_back(line_buffer);
                line_buffer.clear();
            } else if (line_buffer.length() + token.length() <= max_chars_per_line) {
                line_buffer += token;
            } else {
                if (lines.size() >= max_lines) lines.pop_front();
                lines.push_back(line_buffer);
                line_buffer = token;
            }
        }

        // Drawing Logic
        it.fill(esphome::display::COLOR_OFF);
        
        int y = 0;
        for (const auto& line : lines) {
            it.print(0, y, font, color_text, line.c_str());
            y += 16;
        }
        // Draw the temporary line_buffer if it's not empty (typewriter effect)
        if (!line_buffer.empty()) {
            it.print(0, y, font, color_text, line_buffer.c_str());
        }
    }
};

} // namespace watch_assistant

// Global instance to persist state
static watch_assistant::WatchRenderer renderer;