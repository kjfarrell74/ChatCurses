#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>

// ProviderConfig encapsulates configuration for a single AI provider
class ProviderConfig {
public:
    ProviderConfig(std::string id, std::string display_name, std::string default_model, std::vector<std::string> models, std::string api_key_field)
        : id_(std::move(id)), display_name_(std::move(display_name)), default_model_(std::move(default_model)), models_(std::move(models)), api_key_field_(std::move(api_key_field)) {}

    const std::string& id() const { return id_; }
    const std::string& display_name() const { return display_name_; }
    const std::string& default_model() const { return default_model_; }
    const std::vector<std::string>& models() const { return models_; }
    const std::string& api_key_field() const { return api_key_field_; }

private:
    std::string id_;
    std::string display_name_;
    std::string default_model_;
    std::vector<std::string> models_;
    std::string api_key_field_;
};

// Singleton registry for all providers
class ProviderRegistry {
public:
    static ProviderRegistry& instance() {
        static ProviderRegistry inst;
        return inst;
    }

    const ProviderConfig& get(const std::string& id) const {
        auto it = providers_.find(id);
        if (it == providers_.end()) throw std::out_of_range("Provider not found: " + id);
        return *(it->second);
    }

    std::vector<std::string> provider_ids() const {
        std::vector<std::string> ids;
        for (const auto& [id, _] : providers_) ids.push_back(id);
        return ids;
    }

    // Returns the API key field name for a provider
    std::string api_key_field(const std::string& id) const {
        return get(id).api_key_field();
    }

    // Returns the display name for a provider
    std::string display_name(const std::string& id) const {
        return get(id).display_name();
    }

    // Returns the default model for a provider
    std::string default_model(const std::string& id) const {
        return get(id).default_model();
    }

    // Returns the available models for a provider
    const std::vector<std::string>& models(const std::string& id) const {
        return get(id).models();
    }

private:
    ProviderRegistry() {
        // Register all supported providers here
        providers_["xai"] = std::make_unique<ProviderConfig>(
            "xai", "xAI", "grok-3-beta", std::vector<std::string>{"grok-3-beta", "grok-1", "grok-1.5"}, "xai_api_key");
        providers_["claude"] = std::make_unique<ProviderConfig>(
            "claude", "Claude", "claude", std::vector<std::string>{"claude", "claude-3-opus-20240229", "claude-3-sonnet-20240229"}, "claude_api_key");
        providers_["openai"] = std::make_unique<ProviderConfig>(
            "openai", "OpenAI", "gpt-4o", std::vector<std::string>{"gpt-4o", "gpt-4", "gpt-3.5-turbo"}, "openai_api_key");
        providers_["gemini"] = std::make_unique<ProviderConfig>(
            "gemini", "Gemini", "gemini-1.5-pro", std::vector<std::string>{"gemini-1.5-pro", "gemini-1.5-flash", "gemini-2.0-flash-thinking-exp", "gemini-2.0-flash-exp"}, "gemini_api_key");
    }
    ProviderRegistry(const ProviderRegistry&) = delete;
    ProviderRegistry& operator=(const ProviderRegistry&) = delete;

    std::map<std::string, std::unique_ptr<ProviderConfig>> providers_;
};
