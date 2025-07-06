# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands
- Build project: `cmake -B build && cmake --build build`
- Run application: `./build/chatbot`
- Clean build: `rm -rf build && cmake -B build`

## Architecture Overview
ChatCurses is a terminal-based AI chatbot application with a modular architecture:

### Core Components
- **ChatbotApp**: Main application using PIMPL pattern (ChatbotAppImpl handles implementation)
- **NCursesUI**: Terminal interface with RAII NcursesWindow wrapper
- **SettingsPanel**: Configuration UI for providers and API keys
- **MessageHandler**: Thread-safe message queue with streaming support
- **ConfigManager**: JSON configuration persistence using std::expected

### AI Provider System
- **AIClientInterface**: Pure virtual interface for AI providers
- **BaseAIClient**: Abstract base with common functionality (thread-safe config, history)
- **Concrete Clients**: XAIClient, ClaudeAIClient, OpenAIClient, MCPClient
- **ProviderRegistry**: Singleton managing all supported providers and their models
- **ProviderConfig**: Individual provider configuration data

### Key Architectural Patterns
- **PIMPL**: ChatbotApp hides implementation complexity
- **Strategy**: Pluggable AI providers through common interface
- **Provider Registry**: Dynamic provider/model selection at runtime
- **Result-based APIs**: Using std::expected<T, Error> for error handling

## Code Style Guidelines
- C++23 standard with modern features (std::format, std::expected)
- Use snake_case for variables, methods, and file names
- Use PascalCase for class/struct names and enum values
- Include guards: Use #pragma once
- Organization: Headers in include/, implementation in src/
- Error handling: Use result-based API with error enums and descriptive messages
- Logging: Use RichLogger (global instance accessed via get_logger()) with appropriate log levels
- Prefer std::string over char*, std::format over sprintf
- Member variables: Suffix with underscore (e.g., running_)
- Threading: Use std::thread with detach() for async operations
- Includes: Group standard library includes separately from project includes
- Prefer nullptr over NULL or 0

## Adding New AI Providers
1. Create new client class inheriting from BaseAIClient
2. Implement AIClientInterface methods (send_message, etc.)
3. Add provider configuration to ProviderRegistry
4. Define required API key fields and model list
5. Update SettingsPanel to handle new provider configuration