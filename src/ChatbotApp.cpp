#include "ChatbotApp.hpp"
#include "NCursesUI.hpp"
#include "SettingsPanel.hpp"
#include "ProviderConfig.hpp"
#include "XAIClient.hpp"
#include "ClaudeAIClient.hpp"
#include "OpenAIClient.hpp"
#include "GeminiAIClient.hpp"
#include "MessageHandler.hpp"
#include "CommandLineEditor.hpp"
#include "GlobalLogger.hpp"
#include "ConfigManager.hpp"
#include "SignalHandler.hpp"
#include "utf8_utils.hpp"
#include "MCPClient.hpp"
#include "MCPService.hpp"
#include "MCPNotificationInterface.hpp"
#include "MCPServerManager.hpp"
#include "MCPToolService.hpp"

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
    GeminiAIClient gemini_client_;

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
        // Set up all AI clients using the registry and settings helpers
        xai_client_.set_api_key(settings_.xai_api_key);
        xai_client_.set_system_prompt(settings_.system_prompt);
        xai_client_.set_model(ProviderRegistry::instance().default_model("xai"));
        xai_client_.clear_history();

        claude_client_.set_api_key(settings_.claude_api_key);
        claude_client_.set_system_prompt(settings_.system_prompt);
        claude_client_.set_model(ProviderRegistry::instance().default_model("claude"));
        claude_client_.clear_history();

        openai_client_.set_api_key(settings_.openai_api_key);
        openai_client_.set_system_prompt(settings_.system_prompt);
        openai_client_.set_model(ProviderRegistry::instance().default_model("openai"));
        openai_client_.clear_history();

        gemini_client_.set_api_key(settings_.gemini_api_key);
        gemini_client_.set_system_prompt(settings_.system_prompt);
        gemini_client_.set_model(ProviderRegistry::instance().default_model("gemini"));
        gemini_client_.clear_history();

        // Initialize MCP server manager
        auto mcp_init_result = mcp_server_manager_.initialize("mcp_config.json");
        if (mcp_init_result.has_value()) {
            get_logger().log(LogLevel::Info, "MCP server manager initialized successfully");
            
            // Connect to all enabled MCP servers
            auto connect_result = mcp_server_manager_.connect_all();
            if (connect_result.has_value()) {
                auto connected_servers = mcp_server_manager_.get_connected_servers();
                get_logger().log(LogLevel::Info, std::format("Connected to {} MCP servers", connected_servers.size()));
                for (const auto& server : connected_servers) {
                    get_logger().log(LogLevel::Info, std::format("  - {}", server));
                }
                
                // Initialize MCP tool service with the server manager
                MCPToolService::instance().initialize(&mcp_server_manager_);
                get_logger().log(LogLevel::Info, "MCP tool service initialized");
                
            } else {
                get_logger().log(LogLevel::Warning, "Some MCP servers failed to connect");
            }
        } else {
            get_logger().log(LogLevel::Warning, "Failed to initialize MCP server manager");
        }

        // Initialize legacy MCP service if configured
        if (!settings_.mcp_server_url.empty()) {
            MCPService::instance().configure(settings_.mcp_server_url);
            get_logger().log(LogLevel::Info, std::format("Legacy MCP service configured for: {}", settings_.mcp_server_url));
        }

        // Initialize Scrapex service if configured
        if (!settings_.scrapex_server_url.empty()) {
            MCPService::instance().configure(settings_.scrapex_server_url);
            get_logger().log(LogLevel::Info, std::format("Scrapex service configured for: {}", settings_.scrapex_server_url));
        }

        SignalHandler::setup([this]() { on_exit(); });
        ui_ = std::make_unique<NCursesUI>();
        settings_panel_.set_visible(false);
        
        // Setup MCP notifications
        setup_mcp_notifications();
    }

    void setup_mcp_notifications() {
        // Set up callbacks for MCP activity notifications
        mcp_notifier_.set_activity_callback([this](const std::string& activity) {
            if (ui_) {
                ui_->show_mcp_activity(activity);
                needs_redraw_ = true;
            }
        });
        
        mcp_notifier_.set_tool_call_start_callback([this](const std::string& tool_name, const nlohmann::json& args) {
            if (ui_) {
                ui_->show_mcp_activity(std::format("Calling tool: {}", tool_name));
                needs_redraw_ = true;
            }
        });
        
        mcp_notifier_.set_tool_call_success_callback([this](const std::string& tool_name, const nlohmann::json& result) {
            if (ui_) {
                ui_->show_mcp_activity(std::format("Tool {} completed", tool_name));
                needs_redraw_ = true;
            }
        });
        
        mcp_notifier_.set_tool_call_error_callback([this](const std::string& tool_name, const std::string& error) {
            if (ui_) {
                ui_->show_mcp_activity(std::format("Tool {} failed: {}", tool_name, error));
                needs_redraw_ = true;
            }
        });
        
        // Register the notifier with MCP service
        MCPService::instance().set_notification_interface(&mcp_notifier_);
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
                            claude_client_.set_api_key(settings_.claude_api_key);
                            claude_client_.set_model(model_to_use);
                            claude_client_.push_user_message(input);
                            auto messages = claude_client_.build_message_history();
                            std::thread([this, messages, model_to_use]() {
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
                            openai_client_.set_api_key(settings_.openai_api_key);
                            openai_client_.set_model(model_to_use);
                            openai_client_.push_user_message(input);
                            auto messages = openai_client_.build_message_history("");
                            std::thread([this, messages, model_to_use]() {
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
                            xai_client_.set_api_key(settings_.xai_api_key);
                            xai_client_.set_model(model_to_use);
                            xai_client_.push_user_message(input);
                            xai_client_.send_message_stream(
                                prompt, model_to_use,
                                [this](const std::string& chunk, bool is_last_chunk) {
                                    message_handler_.append_to_last_ai_message(chunk, is_last_chunk);
                                    if (is_last_chunk) {
                                        xai_client_.push_assistant_message(chunk);
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
                        } else if (settings_.provider == "gemini") {
                            gemini_client_.set_api_key(settings_.gemini_api_key);
                            gemini_client_.set_model(model_to_use);
                            gemini_client_.push_user_message(input);
                            auto messages = gemini_client_.build_message_history("");
                            std::thread([this, messages, model_to_use]() {
                                auto fut = gemini_client_.send_message(messages, model_to_use);
                                auto result = fut.get();
                                if (result) {
                                    std::string reply = *result;
                                    message_handler_.append_to_last_ai_message(reply, true);
                                    gemini_client_.push_assistant_message(reply);
                                } else {
                                    auto error = result.error();
                                    std::string error_msg = std::format("[Gemini Error {}: {}]", static_cast<int>(error.code), error.message);
                                    message_handler_.append_to_last_ai_message(error_msg, true);
                                }
                                waiting_for_ai_ = false;
                                needs_redraw_ = true;
                            }).detach();
                        } else if (settings_.provider == "mcp") {
                            // Handle MCP server communication
                            message_handler_.append_to_last_ai_message("MCP server communication not yet implemented", true);
                            waiting_for_ai_ = false;
                            needs_redraw_ = true;
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
        // Let NCursesUI destructor handle endwin() via RAII pattern
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
    MCPCallbackNotifier mcp_notifier_;
    MCPServerManager mcp_server_manager_;
};

ChatbotApp::ChatbotApp() : impl_(std::make_unique<ChatbotAppImpl>()) {}
ChatbotApp::~ChatbotApp() = default;
void ChatbotApp::run() { impl_->run(); }
