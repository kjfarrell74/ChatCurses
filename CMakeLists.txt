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
    src/MessageHandler.cpp
    src/CommandLineEditor.cpp
    src/Logger.cpp
    src/ConfigManager.cpp
    src/SignalHandler.cpp
    src/utf8_utils.cpp
)

target_include_directories(chatbot PRIVATE include)

target_link_libraries(chatbot PRIVATE ${CURSES_LIBRARIES} Threads::Threads CURL::libcurl)

# Optionally add spdlog, nlohmann_json, or cpp-httplib if available
# target_link_libraries(chatbot PRIVATE spdlog::spdlog nlohmann_json::nlohmann_json httplib::httplib)

# For modern dependency management, consider using FetchContent for header-only libraries:
# include(FetchContent)
# FetchContent_Declare(
#   nlohmann_json
#   GIT_REPOSITORY https://github.com/nlohmann/json.git
#   GIT_TAG v3.11.2
# )
# FetchContent_MakeAvailable(nlohmann_json)