#include "ChatbotApp.hpp"
#include "NCursesUI.hpp"
#include "SettingsPanel.hpp"
#include "XAIClient.hpp"
#include "ClaudeAIClient.hpp"
#include "OpenAIClient.hpp"
#include "MessageHandler.hpp"
#include "CommandLineEditor.hpp"
#include "GlobalLogger.hpp"
#include "ConfigManager.hpp"
#include "SignalHandler.hpp"
#include "utf8_utils.hpp"

#include <atomic>
#include <vector>
#include <string>
#include <chrono>

namespace {
    constexpr int kInputWinHeight = 3;
}

class ChatbotAppImpl {
private:
    ClaudeAIClient claude_client_;
    OpenAIClient openai_client_;

public:
    std::atomic<bool> needs_redraw_{false};
    std::atomic<bool> waiting_for_ai_{false};
public:
    ChatbotAppImpl()
        : config_manager_("chatbot_config.json"),
          settings_panel_(settings_, &config_manager_),
          running_(true),
          scroll_offset_(0)
    {
        auto load_result = config_manager_.load();
        if (load_result) {
            settings_ = *load_result;
            get_logger().log(LogLevel::Info, std::format("Settings loaded successfully from {}", config_manager_.config_path()));
        } else {
            get_logger().log(LogLevel::Error, std::format("Failed to load settings from {}: Error {}", config_manager_.config_path(), static_cast<int>(load_result.error())));
        }
        xai_client_.set_api_key(settings_.xai_api_key);
        xai_client_.set_system_prompt(settings_.system_prompt);
        xai_client_.set_model(settings_.model);
        xai_client_.clear_history(); // Clear conversation history on app start
        claude_client_.set_api_key(settings_.claude_api_key);
        claude_client_.set_system_prompt(settings_.system_prompt);
        claude_client_.set_model(settings_.model);
        claude_client_.clear_history();
        openai_client_.set_api_key(settings_.openai_api_key);
        openai_client_.set_system_prompt(settings_.system_prompt);
        openai_client_.set_model(settings_.model);
        openai_client_.clear_history();
        SignalHandler::setup([this]() { on_exit(); });
        ui_ = std::make_unique<NCursesUI>();
        settings_panel_.set_visible(false);
    }

    void run() {
        constexpr int kGetchTimeoutMs = 100;
        timeout(kGetchTimeoutMs); // Make getch() non-blocking
        int last_message_count = message_handler_.message_count();
        draw();
        while (running_) {
            if (SignalHandler::check_and_clear_resize()) {
                ui_->handle_resize();
                needs_redraw_ = true;
            }
            int ch = getch();
            int current_message_count = message_handler_.message_count();
            bool need_redraw = false;
            if (current_message_count != last_message_count) {
                need_redraw = true;
                last_message_count = current_message_count;
            }
            if (ch != ERR) {
                // Log all keypresses for debugging
                get_logger().log(LogLevel::Debug, std::format("Key pressed: {}", ch));
                if (settings_panel_.is_visible()) {
                    if (ch == 27) { // ESC
                        settings_panel_.set_visible(false);
                        need_redraw = true;
                        continue;
                    }
                    settings_panel_.handle_input(ch);
                    need_redraw = true;
                    needs_redraw_ = true;
                    continue;
                }
                switch (ch) {
                case KEY_F(2):
                    get_logger().log(LogLevel::Info, "F2 pressed, toggling settings panel");
                    settings_panel_.set_visible(!settings_panel_.is_visible());
                    get_logger().log(LogLevel::Debug, std::string("After toggle, settings_panel_.is_visible() = ") + (settings_panel_.is_visible() ? "true" : "false"));
                    needs_redraw_ = true;
                    break;
                case KEY_UP:
                    scroll_offset_++; // We'll clamp it later in draw()
                    break;
                case KEY_DOWN:
                    scroll_offset_--; // Clamp later
                    break;
                case 10: // Enter
                case KEY_ENTER:
                {
                    std::string input = input_editor_.current_line();
                    if (!input.empty()) {
                        message_handler_.push_message({ChatMessage::Sender::User, input});
                        input_editor_.add_history(input);
                        input_editor_.clear();
                        waiting_for_ai_ = true;
                        needs_redraw_ = true; // Show waiting indicator immediately
                        message_handler_.push_message({ChatMessage::Sender::AI, ""}); // Add placeholder for AI response
                        std::string prompt = input;
                        std::string model_to_use = settings_.model;
                        if (settings_.provider == "claude") {
                            claude_client_.push_user_message(input);
                            auto messages = claude_client_.build_message_history();
                            std::thread([=]() {
                                auto fut = claude_client_.send_message(messages, model_to_use);
                                auto result = fut.get();
                                if (result) {
                                    std::string reply = *result;
                                    message_handler_.append_to_last_ai_message(reply, true);
                                    claude_client_.push_assistant_message(reply);
                                } else {
                                    auto error = result.error();
                                    std::string error_msg = std::format("[Error {}: {}]", static_cast<int>(error.code), error.message);
                                    message_handler_.append_to_last_ai_message(error_msg, true);
                                }
                                waiting_for_ai_ = false;
                                needs_redraw_ = true;
                            }).detach();
                        } else if (settings_.provider == "openai") {
                            openai_client_.push_user_message(input);
                            auto messages = openai_client_.build_message_history("");
                            std::thread([=]() {
                                auto fut = openai_client_.send_message(messages, model_to_use);
                                auto result = fut.get();
                                if (result) {
                                    std::string reply = *result;
                                    message_handler_.append_to_last_ai_message(reply, true);
                                    openai_client_.push_assistant_message(reply);
                                } else {
                                    auto error = result.error();
std::string error_msg = std::format("[OpenAI Error {}: {}]", static_cast<int>(error.code), error.message);
message_handler_.append_to_last_ai_message(error_msg, true);
                                }
                                waiting_for_ai_ = false;
                                needs_redraw_ = true;
                            }).detach();
                        } else if (settings_.provider == "xai") {
                            xai_client_.push_user_message(input); // Add to conversation history
                            xai_client_.send_message_stream(
                                prompt, model_to_use,
                                [this](const std::string& chunk, bool is_last_chunk) {
                                    message_handler_.append_to_last_ai_message(chunk, is_last_chunk);
                                    if (is_last_chunk) {
                                        xai_client_.push_assistant_message(chunk); // Add AI response to history
                                    }
                                    needs_redraw_ = true;
                                },
                                [this]() {
                                    waiting_for_ai_ = false;
                                    needs_redraw_ = true;
                                },
                                [this](const ApiErrorInfo& error) {
                                    std::string error_msg = std::format("[Error {}: {}]", static_cast<int>(error.code), error.message);
                                    message_handler_.append_to_last_ai_message(error_msg, true);
                                    get_logger().log(LogLevel::Error, std::format("API Error: {} - {}", static_cast<int>(error.code), error.message));
                                    waiting_for_ai_ = false;
                                    needs_redraw_ = true;
                                }
                            );
                        }
                    }
                    break;
                }
                case 9: // Tab
                    // Optionally switch focus
                    break;
                case KEY_RESIZE:
                    ui_->handle_resize();
                    break;
                case KEY_PPAGE: // PageUp
                    scroll_offset_ += 5; // Scroll up by 5 lines
                    if (scroll_offset_ < 0) scroll_offset_ = 0;
                    need_redraw = true;
                    break;
                case KEY_NPAGE: // PageDown
                    scroll_offset_ -= 5; // Scroll down by 5 lines
                    if (scroll_offset_ < 0) scroll_offset_ = 0;
                    need_redraw = true;
                    break;
                case 24: // Ctrl+X quit
                    running_ = false;
                    break;
                default:
                    input_editor_.handle_input(ch);
                    need_redraw = true;
                    break;
            }
            }
            if (need_redraw || needs_redraw_) {
                draw();
                needs_redraw_ = false;
            }
        }
        on_exit();
    }

    void draw() {
        if (settings_panel_.is_visible()) {
            settings_panel_.draw(ui_->get_settings_win());
            return;
        }
        std::vector<std::string> raw_lines;
        auto messages = message_handler_.get_messages(0, message_handler_.message_count());
        int maxy, maxx;
        getmaxyx(stdscr, maxy, maxx);
        int chat_win_width = maxx - 2; // Adjust for borders if any
        int wrap_indent = 2; // Indent for wrapped lines
        for (const auto& msg : messages) {
            std::string prefix = (msg.sender == ChatMessage::Sender::User) ? settings_.user_display_name + ": " : "AI: ";
            std::istringstream iss(msg.content);
            std::string line;
            bool first = true;
            while (std::getline(iss, line)) {
                raw_lines.push_back((first ? prefix : std::string(prefix.size(), ' ')) + line);
                first = false;
            }
        }
        std::vector<std::string_view> lines;
        lines.reserve(raw_lines.size());
        for (const auto& s : raw_lines) lines.push_back(s);
        int total_lines = ui_->draw_chat_window(lines, scroll_offset_, waiting_for_ai_);
        // Clamp scroll_offset_ so we never scroll past start or end

        int display_lines = maxy - 2;
        int max_scroll = std::max(0, total_lines - display_lines);
        if (scroll_offset_ > max_scroll) scroll_offset_ = max_scroll;
        if (scroll_offset_ < 0) scroll_offset_ = 0;
        ui_->draw_input_window(input_editor_.current_line(), input_editor_.cursor_pos());
        ui_->refresh_all();
    }

    void on_exit() {
        // Robust curses cleanup: always call endwin() safely
        static bool exited = false;
        if (exited) return;
        exited = true;
        config_manager_.save(settings_);
        running_ = false;
        // Defensive: call endwin() directly in case NCursesUI is destroyed
        endwin();
    }

private:

    ConfigManager config_manager_;
    Settings settings_;
    SettingsPanel settings_panel_;
    XAIClient xai_client_;
    MessageHandler message_handler_;
    CommandLineEditor input_editor_;
    std::unique_ptr<NCursesUI> ui_;
    std::atomic<bool> running_;
    int scroll_offset_;
};

ChatbotApp::ChatbotApp() : impl_(std::make_unique<ChatbotAppImpl>()) {}
ChatbotApp::~ChatbotApp() = default;
void ChatbotApp::run() { impl_->run(); }
