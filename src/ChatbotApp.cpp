#include "ChatbotApp.hpp"
#include "NCursesUI.hpp"
#include "SettingsPanel.hpp"
#include "XAIClient.hpp"
#include "MessageHandler.hpp"
#include "CommandLineEditor.hpp"
#include "Logger.hpp"
#include "ConfigManager.hpp"
#include "SignalHandler.hpp"
#include <thread>
#include <atomic>
#include <vector>
#include <string>
#include <chrono>

namespace {
    constexpr int kInputWinHeight = 3;
}

class ChatbotAppImpl {
public:
    std::atomic<bool> needs_redraw_{false};
    std::atomic<bool> waiting_for_ai_{false};
public:
    ChatbotAppImpl()
        : logger_("chatbot.log"),
          config_manager_("chatbot_config.json"),
          settings_panel_(settings_, &config_manager_),
          running_(true),
          scroll_offset_(0) {
        config_manager_.load(settings_);
        xai_client_.set_api_key(settings_.api_key);
        xai_client_.set_system_prompt(settings_.system_prompt);
        xai_client_.set_model(settings_.ai_model);
        SignalHandler::setup([this]() { on_exit(); });
        ui_ = std::make_unique<NCursesUI>();
        settings_panel_.set_visible(false);
    }

    void run() {
        timeout(100); // Make getch() non-blocking with 100ms timeout
        int last_message_count = message_handler_.message_count();
        draw();
        while (running_) {
            // Handle terminal resize
            if (SignalHandler::check_and_clear_resize()) {
                ui_->handle_resize();
                needs_redraw_ = true;
            }
            int ch = getch();
            int current_message_count = message_handler_.message_count();
            bool need_redraw = false;
            // Redraw if new message arrived
            if (current_message_count != last_message_count) {
                need_redraw = true;
                last_message_count = current_message_count;
            }
            if (ch != ERR) {
                if (settings_panel_.is_visible()) {
                    if (ch == 27) { // ESC
                        settings_panel_.set_visible(false);
                        need_redraw = true;
                        continue;
                    }
                    settings_panel_.handle_input(ch);
                    need_redraw = true;
                    continue;
                }
                switch (ch) {
                case KEY_F(2):
                    settings_panel_.set_visible(!settings_panel_.is_visible());
                    break;
                case KEY_UP:
                    scroll_offset_ = std::min(scroll_offset_ + 1, std::max(0, message_handler_.message_count() - 1));
                    break;
                case KEY_DOWN:
                    scroll_offset_ = std::max(0, scroll_offset_ - 1);
                    break;
                case 10: // Enter
                case KEY_ENTER:
                {
                    std::string input = input_editor_.current_line();
                    if (!input.empty()) {
                        message_handler_.push_message({ChatMessage::Sender::User, input});
                        input_editor_.add_history(input);
                        input_editor_.clear();
                        // Async send to xAI
                        std::string prompt = input;
                        waiting_for_ai_ = true;
                        message_handler_.push_message({ChatMessage::Sender::AI, ""});
                        xai_client_.send_message_stream(
                            prompt, settings_.ai_model,
                            [this](const std::string& chunk, bool is_last_chunk) {
                                message_handler_.append_to_last_ai_message(chunk, is_last_chunk);
                                needs_redraw_ = true;
                            },
                            [this]() {
                                waiting_for_ai_ = false;
                                needs_redraw_ = true;
                            }
                        );
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
        std::vector<std::string> lines;
        auto messages = message_handler_.get_messages(0, message_handler_.message_count());
        for (const auto& msg : messages) {
            std::string prefix = (msg.sender == ChatMessage::Sender::User) ? settings_.user_display_name + ": " : "AI: ";
            lines.push_back(prefix + msg.content);
        }
        int total_lines = ui_->draw_chat_window(lines, scroll_offset_, waiting_for_ai_);
        // Clamp scroll_offset_ so we never scroll past start or end
        int maxy, maxx;
        getmaxyx(stdscr, maxy, maxx);
        int display_lines = maxy - 2;
        int max_scroll = std::max(0, total_lines - display_lines);
        if (scroll_offset_ > max_scroll) scroll_offset_ = max_scroll;
        if (scroll_offset_ < 0) scroll_offset_ = 0;
        ui_->draw_input_window(input_editor_.current_line(), input_editor_.cursor_pos());
        if (settings_panel_.is_visible()) {
            settings_panel_.draw();
        }
        ui_->refresh_all();
    }

    void on_exit() {
        config_manager_.save(settings_);
        running_ = false;
    }

private:
    Logger logger_;
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
