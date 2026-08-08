import json, socket, uuid

MAGIC = "ue_py"
VERSION = 1
node_id = str(uuid.uuid4())
GROUP = ("239.0.0.1", 6766)
BIND = "127.0.0.1"

msg = json.dumps({"version": VERSION, "magic": MAGIC, "type": "ping", "source": node_id}).encode("utf-8")

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
try:
    sock.bind((BIND, 0))
except Exception as e:
    print("bind fail", e)
    sock.bind(("0.0.0.0", 0))

try:
    mreq = socket.inet_aton(GROUP[0]) + socket.inet_aton(BIND)
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
except Exception as e:
    print("mcast join fail", e)
try:
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_IF, socket.inet_aton(BIND))
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 0)
except Exception as e:
    print("mcast if fail", e)

sock.settimeout(1.0)
print("local", sock.getsockname(), "sending ping to", GROUP)
got = False
for i in range(8):
    sock.sendto(msg, GROUP)
    try:
        data, addr = sock.recvfrom(65535)
        print("GOT", addr, data[:800])
        got = True
        break
    except socket.timeout:
        print("timeout", i)

if not got:
    print("retry unicast 127.0.0.1")
    for i in range(5):
        sock.sendto(msg, ("127.0.0.1", 6766))
        try:
            data, addr = sock.recvfrom(65535)
            print("GOT unicast", addr, data[:800])
            got = True
            break
        except socket.timeout:
            print("uc timeout", i)

if not got:
    print("NO_NODES")
