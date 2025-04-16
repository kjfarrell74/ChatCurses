#pragma once
#include <string>
#include <future>
#include <vector>
#include <concepts>
#include <coroutine>
#include <expected> // C++23
#include <functional> // For std::function

enum class ApiError {
    CurlInitFailed,
    ApiKeyNotSet,
    NetworkError, // Contains curl error string
    JsonParseError, // Contains exception what() string
    MalformedResponse,
    Unknown
};

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
        std::function<void(const std::string& chunk, bool is_last_chunk)> on_chunk_cb,
        std::function<void()> on_done_cb,
        std::function<void(const ApiErrorInfo& error)> on_error_cb);

    XAIClient();
    void set_api_key(const std::string& key);
    void set_system_prompt(const std::string& prompt);
    void set_model(const std::string& model);
    std::future<std::expected<std::string, ApiErrorInfo>> send_message(const std::string& prompt, const std::string& model = "");
    std::vector<std::string> available_models() const;
    // Deprecated: Use the expected return value instead
    // std::string last_error() const;
private:
    std::string api_key_;
    std::string system_prompt_;
    std::string model_;
};

// Structure to hold specific error details
struct ApiErrorInfo {
    ApiError code = ApiError::Unknown;
    std::string details; // e.g., curl error string or exception message
};
