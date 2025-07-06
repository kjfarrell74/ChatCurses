#!/bin/bash

# Test the websocketd bridge by sending a raw message

echo "Testing websocketd bridge on localhost:9090"
echo "Sending initialize request..."

# Create a test message
MESSAGE='{"jsonrpc": "2.0", "id": 1, "method": "initialize", "params": {"protocolVersion": "2024-11-05", "capabilities": {"sampling": {}}, "clientInfo": {"name": "test-client", "version": "1.0.0"}}}'

# Send the message using nc (netcat)
echo "$MESSAGE" | nc -C localhost 9090