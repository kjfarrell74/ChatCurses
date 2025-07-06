import asyncio
import json
import logging
from pathlib import Path
import websockets

CONFIG_FILE = Path("mcp_server_config.json")

MCP_PROTOCOL_VERSION = "2024-11-05"


def load_config():
    config = {"host": "localhost", "port": 9090}
    if CONFIG_FILE.exists():
        try:
            with open(CONFIG_FILE) as f:
                data = json.load(f)
            if isinstance(data, dict):
                config.update({k: data[k] for k in ("host", "port") if k in data})
        except Exception as e:
            logging.warning("Failed to read %s: %s", CONFIG_FILE, e)
    return config

async def handle_connection(websocket):
    logging.info("Client connected")
    async for message in websocket:
        logging.debug("Received: %s", message)
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
            logging.debug("Sent initialize response")
        elif method == "shutdown":
            resp = {"jsonrpc": "2.0", "id": msg_id, "result": {}}
            await websocket.send(json.dumps(resp))
            logging.debug("Sent shutdown ack")
            await websocket.close()
        elif method == "ping":
            resp = {"jsonrpc": "2.0", "id": msg_id, "result": {}}
            await websocket.send(json.dumps(resp))
            logging.debug("Sent ping response")
        elif method == "tools/list":
            resp = {
                "jsonrpc": "2.0",
                "id": msg_id,
                "result": {"tools": [{"name": "hello_world", "description": "Return greeting"}]}
            }
            await websocket.send(json.dumps(resp))
            logging.debug("Sent tools list")
        elif method == "tools/call":
            name = params.get("name")
            if name == "hello_world":
                result = {"message": "Hello, MCP!"}
            else:
                result = {"error": "unknown tool"}
            resp = {"jsonrpc": "2.0", "id": msg_id, "result": result}
            await websocket.send(json.dumps(resp))
            logging.debug("Sent tool result")
        else:
            # Unknown method
            resp = {
                "jsonrpc": "2.0",
                "id": msg_id,
                "error": {"code": -32601, "message": "Method not found"}
            }
            await websocket.send(json.dumps(resp))
            logging.debug("Sent method not found error")

async def main():
    cfg = load_config()
    host = cfg.get("host", "localhost")
    port = cfg.get("port", 9090)
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
    logging.info("Starting Hello MCP server on %s:%s", host, port)
    async with websockets.serve(handle_connection, host, port):
        await asyncio.Future()  # run forever

if __name__ == "__main__":
    asyncio.run(main())
