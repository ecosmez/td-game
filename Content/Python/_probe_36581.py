import socket, time, json

s = socket.socket()
s.settimeout(3)
try:
    s.connect(("127.0.0.1", 36581))
    print("connected")
except Exception as e:
    print("connect fail", e)
    raise SystemExit(1)

payload = json.dumps({
    "jsonrpc": "2.0",
    "id": 1,
    "method": "initialize",
    "params": {
        "protocolVersion": "2024-11-05",
        "capabilities": {},
        "clientInfo": {"name": "c", "version": "1"},
    },
}) + "\n"
s.sendall(payload.encode())
time.sleep(0.5)
try:
    data = s.recv(8192)
    print("recv", data[:1000])
except Exception as e:
    print("recv fail", e)

s2 = socket.socket()
s2.settimeout(5)
s2.connect(("127.0.0.1", 36581))
body = json.dumps({
    "jsonrpc": "2.0",
    "id": 1,
    "method": "initialize",
    "params": {
        "protocolVersion": "2024-11-05",
        "capabilities": {},
        "clientInfo": {"name": "c", "version": "1"},
    },
})
req = (
    "POST /mcp HTTP/1.1\r\n"
    "Host: 127.0.0.1:36581\r\n"
    "Content-Type: application/json\r\n"
    "Accept: application/json, text/event-stream\r\n"
    f"Content-Length: {len(body)}\r\n"
    "\r\n"
    f"{body}"
)
s2.sendall(req.encode())
time.sleep(1)
try:
    chunks = []
    while True:
        try:
            part = s2.recv(8192)
            if not part:
                break
            chunks.append(part)
            if len(b"".join(chunks)) > 2000:
                break
        except socket.timeout:
            break
    data = b"".join(chunks)
    print("http recv", data[:2000])
except Exception as e:
    print("http recv fail", e)
