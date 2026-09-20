#!/usr/bin/env python3

"""Minimal UDP client for ckv."""

import socket
import sys


def request(client, address, command):
    client.sendto(command.encode("ascii"), address)
    response, _ = client.recvfrom(4096)
    return response.decode("ascii").rstrip("\n")


def main():
    port = int(sys.argv[1]) if len(sys.argv) == 2 else 6379
    if len(sys.argv) > 2:
        raise SystemExit(f"usage: {sys.argv[0]} [port]")

    address = ("127.0.0.1", port)
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as client:
        client.settimeout(2)
        for command in ("SET example hello world", "GET example", "DEL example", "QUIT"):
            print(f"{command} -> {request(client, address, command)}")


if __name__ == "__main__":
    main()
