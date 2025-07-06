#!/bin/bash
# Setup MCP servers for Claude Code
# Based on configuration from ~/.gemini/settings.json

set -e

echo "Setting up MCP servers for Claude Code..."

# Context7 server
echo "Adding context7 server..."
claude mcp add context7 npx @upstash/context7-mcp

# Sequential thinking server
echo "Adding sequentialthinking server..."
claude mcp add sequentialthinking npx @modelcontextprotocol/server-sequential-thinking

# Filesystem server
echo "Adding filesystem server..."
claude mcp add filesystem npx @modelcontextprotocol/server-filesystem /home/kfarrell

# GitHub server
echo "Adding github server..."
claude mcp add github npx @modelcontextprotocol/server-github

# Playwright server
echo "Adding playwright server..."
claude mcp add playwright npx @playwright/mcp@latest

echo "MCP servers setup complete!"
echo "Use 'claude mcp list' to verify all servers are configured."





