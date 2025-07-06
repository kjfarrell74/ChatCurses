import asyncio
import json
import websockets

MCP_PROTOCOL_VERSION = "2024-11-05"

async def handle_connection(websocket):
    async for message in websocket:
        try:
            request = json.loads(message)
        except json.JSONDecodeError:
            continue

        method = request.get("method")
        msg_id = request.get("id")
        params = request.get("params", {})

        # Ignore notifications (no id)
        if msg_id is None and method in {"notifications/initialized", "initialized"}:
            continue

        if method == "initialize":
            response = {
                "jsonrpc": "2.0",
                "id": msg_id,
                "result": {
                    "protocolVersion": MCP_PROTOCOL_VERSION,
                    "capabilities": {"tools": {"listChanged": False}},
                    "serverInfo": {"name": "HelloMCP", "version": "0.1"}
                }
            }
            await websocket.send(json.dumps(response))
        elif method == "shutdown":
            resp = {"jsonrpc": "2.0", "id": msg_id, "result": {}}
            await websocket.send(json.dumps(resp))
            await websocket.close()
        elif method == "ping":
            resp = {"jsonrpc": "2.0", "id": msg_id, "result": {}}
            await websocket.send(json.dumps(resp))
        elif method == "tools/list":
            resp = {
                "jsonrpc": "2.0",
                "id": msg_id,
                "result": {"tools": [{"name": "hello_world", "description": "Return greeting"}]}
            }
            await websocket.send(json.dumps(resp))
        elif method == "tools/call":
            name = params.get("name")
            if name == "hello_world":
                result = {"message": "Hello, MCP!"}
            else:
                result = {"error": "unknown tool"}
            resp = {"jsonrpc": "2.0", "id": msg_id, "result": result}
            await websocket.send(json.dumps(resp))
        else:
            # Unknown method
            resp = {
                "jsonrpc": "2.0",
                "id": msg_id,
                "error": {"code": -32601, "message": "Method not found"}
            }
            await websocket.send(json.dumps(resp))

async def main():
    async with websockets.serve(handle_connection, "localhost", 9090):
        await asyncio.Future()  # run forever

if __name__ == "__main__":
    asyncio.run(main())
