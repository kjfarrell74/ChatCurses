#!/usr/bin/env python3
"""
Direct MCP protocol test script
"""
import asyncio
import json
import websockets
import logging

logging.basicConfig(level=logging.DEBUG)

async def test_mcp_connection():
    uri = "ws://localhost:9090"
    
    try:
        async with websockets.connect(uri) as websocket:
            print("Connected to MCP server")
            
            # Send initialize request
            init_request = {
                "jsonrpc": "2.0",
                "id": 1,
                "method": "initialize",
                "params": {
                    "protocolVersion": "2024-11-05",
                    "capabilities": {
                        "sampling": {}
                    },
                    "clientInfo": {
                        "name": "test-client",
                        "version": "1.0.0"
                    }
                }
            }
            
            await websocket.send(json.dumps(init_request))
            print("Sent initialize request")
            
            # Wait for response
            response = await websocket.recv()
            print(f"Initialize response: {response}")
            
            # Send initialized notification
            init_notification = {
                "jsonrpc": "2.0",
                "method": "notifications/initialized",
                "params": {}
            }
            
            await websocket.send(json.dumps(init_notification))
            print("Sent initialized notification")
            
            # Send tools/list request
            tools_request = {
                "jsonrpc": "2.0",
                "id": 2,
                "method": "tools/list",
                "params": {}
            }
            
            await websocket.send(json.dumps(tools_request))
            print("Sent tools/list request")
            
            # Wait for response
            response = await websocket.recv()
            print(f"Tools response: {response}")
            
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    asyncio.run(test_mcp_connection())