#!/usr/bin/env python3

"""Minimal TCP client for ckv."""

import socket
import sys


def request(client, command):
    client.sendall((command + "\n").encode("ascii"))
    response = bytearray()
    while not response.endswith(b"\n"):
        chunk = client.recv(4096)
        if not chunk:
            raise ConnectionError("ckv closed the TCP connection")
        response.extend(chunk)
    return response.decode("ascii").rstrip("\n")


def main():
    port = int(sys.argv[1]) if len(sys.argv) == 2 else 6379
    if len(sys.argv) > 2:
        raise SystemExit(f"usage: {sys.argv[0]} [port]")

    with socket.create_connection(("127.0.0.1", port), timeout=2) as client:
        for command in ("SET example hello world", "GET example", "DEL example", "QUIT"):
            print(f"{command} -> {request(client, command)}")


if __name__ == "__main__":
    main()
