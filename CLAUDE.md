# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands
- Build project: `cmake -B build && cmake --build build`
- Run application: `./build/chatbot`
- Clean build: `rm -rf build && cmake -B build`

## Code Style Guidelines
- C++23 standard with modern features (std::format, source_location)
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