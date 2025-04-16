#pragma once
#include <string>
#include <future>
#include <vector>
#include <concepts>
#include <coroutine>

// Concept for AI provider extensibility

template<typename T>
concept AIProvider = requires(T a, const std::string& prompt, const std::string& model) {
    { a.send_message(prompt, model) } -> std::same_as<std::future<std::string>>;
    { a.set_api_key(std::string{}) };
    { a.set_system_prompt(std::string{}) };
    { a.set_model(std::string{}) };
};

class XAIClient {
public:
    void send_message_stream(
        const std::string& prompt,
        const std::string& model,
        std::function<void(const std::string& chunk, bool is_last_chunk)> on_chunk,
        std::function<void()> on_done);

    XAIClient();
    void set_api_key(const std::string& key);
    void set_system_prompt(const std::string& prompt);
    void set_model(const std::string& model);
    std::future<std::string> send_message(const std::string& prompt, const std::string& model = "");
    std::vector<std::string> available_models() const;
    // Error handling
    std::string last_error() const;
private:
    std::string api_key_;
    std::string system_prompt_;
    std::string model_;
    std::string last_error_;
};
