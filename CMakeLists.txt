cmake_minimum_required(VERSION 3.22)
project(Chatbot2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Curses REQUIRED)
find_package(Threads REQUIRED)
find_package(CURL REQUIRED)

add_executable(chatbot
    src/main.cpp
    src/ChatbotApp.cpp
    src/NCursesUI.cpp
    src/SettingsPanel.cpp
    src/ClaudeAIClient.cpp
    src/XAIClient.cpp
    src/OpenAIClient.cpp
    src/GeminiAIClient.cpp
    src/MessageHandler.cpp
    src/CommandLineEditor.cpp
    src/Logger.cpp
    src/ConfigManager.cpp
    src/SignalHandler.cpp
    src/utf8_utils.cpp
    src/MCPClient.cpp
    src/MCPMessage.cpp
    src/MCPProtocol.cpp
    src/MCPResourceManager.cpp
    src/MCPToolManager.cpp
    src/MCPPromptManager.cpp
    src/MCPService.cpp
)

target_include_directories(chatbot PRIVATE include build/_deps/ixwebsocket-src build/_deps/ixwebsocket-src/ixwebsocket)

target_link_libraries(chatbot PRIVATE ${CURSES_LIBRARIES} Threads::Threads CURL::libcurl ixwebsocket)

# Optionally add spdlog, nlohmann_json, or cpp-httplib if available
# target_link_libraries(chatbot PRIVATE spdlog::spdlog nlohmann_json::nlohmann_json httplib::httplib)

# For modern dependency management, consider using FetchContent for header-only libraries:
include(FetchContent)
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)
set(CMAKE_POLICY_DEFAULT_CMP0074 NEW)
FetchContent_MakeAvailable(nlohmann_json)

target_link_libraries(chatbot PRIVATE nlohmann_json::nlohmann_json)

FetchContent_Declare(
  ixwebsocket
  GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
  GIT_TAG v11.4.6
)
FetchContent_MakeAvailable(ixwebsocket)
target_include_directories(chatbot PRIVATE include build/_deps/ixwebsocket-src/ixwebsocket)

# Add MCP tools test program
add_executable(test_mcp_tools
    test_mcp_tools.cpp
    src/MCPService.cpp
    src/MCPClient.cpp
    src/MCPMessage.cpp
    src/MCPProtocol.cpp
    src/MCPResourceManager.cpp
    src/MCPToolManager.cpp
    src/MCPPromptManager.cpp
    src/Logger.cpp
)

target_include_directories(test_mcp_tools PRIVATE include build/_deps/ixwebsocket-src build/_deps/ixwebsocket-src/ixwebsocket)
target_link_libraries(test_mcp_tools PRIVATE nlohmann_json::nlohmann_json ixwebsocket Threads::Threads)

# Add MCP Brave search test program
add_executable(test_mcp_brave
    test_mcp_brave.cpp
    src/MCPService.cpp
    src/MCPClient.cpp
    src/MCPMessage.cpp
    src/MCPProtocol.cpp
    src/MCPResourceManager.cpp
    src/MCPToolManager.cpp
    src/MCPPromptManager.cpp
    src/Logger.cpp
)

target_include_directories(test_mcp_brave PRIVATE include build/_deps/ixwebsocket-src build/_deps/ixwebsocket-src/ixwebsocket)
target_link_libraries(test_mcp_brave PRIVATE nlohmann_json::nlohmann_json ixwebsocket Threads::Threads)

# Add ScrapeX Bridge test program
add_executable(test_scrapex_bridge
    test_scrapex_bridge.cpp
    src/MCPService.cpp
    src/MCPClient.cpp
    src/MCPMessage.cpp
    src/MCPProtocol.cpp
    src/MCPResourceManager.cpp
    src/MCPToolManager.cpp
    src/MCPPromptManager.cpp
    src/Logger.cpp
)

target_include_directories(test_scrapex_bridge PRIVATE include build/_deps/ixwebsocket-src build/_deps/ixwebsocket-src/ixwebsocket)
target_link_libraries(test_scrapex_bridge PRIVATE nlohmann_json::nlohmann_json ixwebsocket Threads::Threads)

# Add simple MCP test program
add_executable(test_simple_mcp
    test_simple_mcp.cpp
    src/MCPClient.cpp
    src/MCPMessage.cpp
    src/MCPProtocol.cpp
    src/MCPResourceManager.cpp
    src/MCPToolManager.cpp
    src/MCPPromptManager.cpp
    src/Logger.cpp
)

target_include_directories(test_simple_mcp PRIVATE include build/_deps/ixwebsocket-src build/_deps/ixwebsocket-src/ixwebsocket)
target_link_libraries(test_simple_mcp PRIVATE nlohmann_json::nlohmann_json ixwebsocket Threads::Threads)

# Add MCP UI notifications test program
add_executable(test_mcp_ui_notifications
    test_mcp_ui_notifications.cpp
    src/MCPService.cpp
    src/MCPClient.cpp
    src/MCPMessage.cpp
    src/MCPProtocol.cpp
    src/MCPResourceManager.cpp
    src/MCPToolManager.cpp
    src/MCPPromptManager.cpp
    src/Logger.cpp
)

target_include_directories(test_mcp_ui_notifications PRIVATE include build/_deps/ixwebsocket-src build/_deps/ixwebsocket-src/ixwebsocket)
target_link_libraries(test_mcp_ui_notifications PRIVATE nlohmann_json::nlohmann_json ixwebsocket Threads::Threads)