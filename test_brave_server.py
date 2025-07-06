import asyncio
import websockets
import json

async def test_brave_search():
    uri = "ws://localhost:9092"
    async with websockets.connect(uri) as websocket:
        request = {
            "id": "1",
            "method": "tools.call",
            "params": {
                "name": "bravesearch",
                "arguments": {
                    "query": "iran news"
                }
            }
        }
        await websocket.send(json.dumps(request))
        response = await websocket.recv()
        print(f"Received response: {response}")

if __name__ == "__main__":
    asyncio.run(test_brave_search())
