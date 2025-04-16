#pragma once
#include <string>

// Unified API error enum for all AI clients

enum class ApiError {
    None,              // For OpenAI
    CurlInitFailed,
    ApiKeyNotSet,
    NetworkError,      // For xAI
    JsonParseError,    // For xAI
    CurlRequestFailed, // For OpenAI
    MalformedResponse,
    Unknown
};

struct ApiErrorInfo {
    ApiError code = ApiError::Unknown;
    std::string message;
    // For backward compatibility
    ApiError error() const { return code; }
    const std::string& details() const { return message; }
};
