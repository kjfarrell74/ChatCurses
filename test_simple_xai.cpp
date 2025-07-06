#include <iostream>
#include <format>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    static_cast<std::string*>(userp)->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

int main() {
    // Simple test to verify XAI API works
    std::string api_key = "xai-6uG5BbihYy5xBLBfPgX2ZU7at8RxzznHDogzWuPHOGOlrJCPF3CpQTN05vZbkXYytrGhJJoAH0RBXqV1";
    
    nlohmann::json request_body;
    request_body["model"] = "grok-3-beta";
    request_body["temperature"] = 0.7;
    request_body["max_tokens"] = 100;
    
    nlohmann::json messages = nlohmann::json::array();
    messages.push_back({
        {"role", "user"},
        {"content", "Hello, how are you?"}
    });
    
    request_body["messages"] = messages;
    
    std::cout << "Request JSON: " << request_body.dump(2) << std::endl;
    
    // Prepare HTTP request
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize curl" << std::endl;
        return 1;
    }
    
    std::string response_string;
    struct curl_slist* headers = nullptr;
    
    // Set headers
    std::string auth_header = "Authorization: Bearer " + api_key;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, auth_header.c_str());
    
    // Set curl options
    std::string json_data = request_body.dump();
    curl_easy_setopt(curl, CURLOPT_URL, "https://api.x.ai/v1/chat/completions");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, json_data.length());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    
    // Perform request
    CURLcode res = curl_easy_perform(curl);
    
    // Check for curl errors
    if (res != CURLE_OK) {
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        std::cerr << "Request failed: " << curl_easy_strerror(res) << std::endl;
        return 1;
    }
    
    // Check HTTP status
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    std::cout << "HTTP Status: " << response_code << std::endl;
    std::cout << "Response: " << response_string << std::endl;
    
    return 0;
}