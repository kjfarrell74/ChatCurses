# ChatCurses

ChatCurses is a terminal-based chatbot application written in C++ that leverages modern AI models for interactive conversations. Designed with a focus on extensibility and usability, ChatCurses provides a curses-based (text UI) interface for seamless chat experiences directly from the command line. The project features modular components for API integration, configuration management, and message handling, making it easy to adapt to different AI backends or extend with new features.

## Key Features
- Clean, responsive terminal UI using curses
- Integration-ready for multiple AI APIs (OpenAI, xAI, Claude)
- Thread-safe configuration and message handling
- Easily customizable system prompts and models
- Robust error handling and logging
- Proper UTF-8 text handling with UTF8-CPP

## Dependencies
- NCurses
- CURL
- UTF8-CPP (included)
- C++23 compatible compiler

## Building
```bash
cmake -B build
cmake --build build
```

## Usage
```bash
./build/chatbot
```

Whether you want a personal AI assistant in your terminal or a foundation for advanced chatbot development, ChatCurses provides a solid, modern C++ base.