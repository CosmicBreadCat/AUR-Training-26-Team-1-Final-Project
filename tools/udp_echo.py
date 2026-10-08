#!/usr/bin/env python3
"""UDP echo test for the ESP32 SoftAP.

Connect the laptop to the "AUR-Team1-Robot" WiFi network first, then run:
    python3 tools/udp_echo.py

Sends MESSAGE once as JSON, waits for the echo, prints it, and exits.
"""
import json
import socket
import sys
import time

HOST = "192.168.4.1"
PORT = 5005
TIMEOUT_S = 1.0

# Edit this to change what gets sent.
MESSAGE = {
    "type": "hello",
    "seq": 1,
    "data": {"v": 0.25, "omega": 0.0},
}


def main():
    payload = json.dumps(MESSAGE).encode()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(TIMEOUT_S)

    print(f"sending {len(payload)} bytes to {HOST}:{PORT}: {payload.decode()}")
    t0 = time.perf_counter()
    sock.sendto(payload, (HOST, PORT))
    try:
        data, src = sock.recvfrom(2048)
    except socket.timeout:
        print(f"no reply within {TIMEOUT_S} s")
        sys.exit(1)
    rtt_ms = (time.perf_counter() - t0) * 1000

    print(f"reply from {src[0]}:{src[1]} ({rtt_ms:.1f} ms)")
    try:
        reply = json.loads(data)
    except json.JSONDecodeError:
        print(f"reply is not valid JSON: {data!r}")
        sys.exit(1)
    print(json.dumps(reply, indent=2))
    print("matches what was sent" if reply == MESSAGE else "DOES NOT match what was sent")


if __name__ == "__main__":
    main()
