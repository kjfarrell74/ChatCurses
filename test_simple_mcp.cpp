#include "MCPClient.hpp"
#include "GlobalLogger.hpp"
#include <iostream>
#include <chrono>
#include <thread>

int main() {
    get_logger().set_level(LogLevel::Debug);
    
    std::cout << "Testing MCP Client with echo server..." << std::endl;
    
    // Create client for websocket echo server
    MCPClient client("ws://localhost:9090");
    
    // Test connection
    std::cout << "Connecting to ws://localhost:9090..." << std::endl;
    auto connect_result = client.connect().get();
    
    if (connect_result) {
        std::cout << "Connected successfully!" << std::endl;
        std::cout << "Connection state: " << static_cast<int>(client.get_connection_state()) << std::endl;
    } else {
        std::cout << "Connection failed: " << connect_result.error().message << std::endl;
    }
    
    // Wait a bit
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Try to disconnect
    client.disconnect().wait();
    
    return 0;
}