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
    ApiError error = ApiError::Unknown; // For OpenAI
    ApiError code = ApiError::Unknown;  // For xAI compatibility
    std::string message;                // For OpenAI
    std::string details;                // For xAI
};
