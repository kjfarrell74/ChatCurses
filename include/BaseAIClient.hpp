#pragma once
#include "AIClientInterface.hpp"

class BaseAIClient : public AIClientInterface {
public:
    void set_api_key(const std::string& key) override {
        std::lock_guard lock(mutex_);
        api_key_ = key;
    }
    
    void set_system_prompt(const std::string& prompt) override {
        std::lock_guard lock(mutex_);
        system_prompt_ = prompt;
    }
    
    void set_model(const std::string& model) override {
        std::lock_guard lock(mutex_);
        model_ = model;
    }
    
    void clear_history() override {
        std::lock_guard lock(mutex_);
        conversation_history_.clear();
    }
    
    void push_user_message(const std::string& content) override {
        std::lock_guard lock(mutex_);
        conversation_history_.push_back({{"role", "user"}, {"content", content}});
    }
    
    void push_assistant_message(const std::string& content) override {
        std::lock_guard lock(mutex_);
        conversation_history_.push_back({{"role", "assistant"}, {"content", content}});
    }
    
protected:
    std::string api_key_;
    std::string system_prompt_;
    std::string model_;
    mutable std::mutex mutex_;
    std::vector<nlohmann::json> conversation_history_;
};