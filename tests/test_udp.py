#!/usr/bin/env python3

import socket
import subprocess
import sys
import time


def tcp_request(port, request):
    with socket.create_connection(("127.0.0.1", port), timeout=2) as client:
        client.sendall((request + "\n").encode())
        return client.recv(4096).decode()


def udp_request(port, request):
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as client:
        client.settimeout(2)
        client.sendto(request.encode(), ("127.0.0.1", port))
        return client.recv(4096).decode()


def main():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as probe:
        probe.bind(("127.0.0.1", 0))
        port = probe.getsockname()[1]

    server = subprocess.Popen(
        ["./ckv", str(port)],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
    )
    try:
        deadline = time.time() + 2
        while time.time() < deadline:
            try:
                if tcp_request(port, "GET startup") == "NOT_FOUND\n":
                    break
            except OSError:
                time.sleep(0.01)
        else:
            raise AssertionError("server did not start on the configured port")

        if udp_request(port, "SET from-udp value") != "OK\n":
            raise AssertionError("UDP SET failed")
        if tcp_request(port, "GET from-udp") != "value\n":
            raise AssertionError("TCP could not read UDP mutation")
        if tcp_request(port, "SET from-tcp other") != "OK\n":
            raise AssertionError("TCP SET failed")
        if udp_request(port, "GET from-tcp") != "other\n":
            raise AssertionError("UDP could not read TCP mutation")
        if udp_request(port, "DEL from-udp") != "OK\n":
            raise AssertionError("UDP DEL failed")
    finally:
        server.terminate()
        try:
            server.wait(timeout=2)
        except subprocess.TimeoutExpired:
            server.kill()
            server.wait()

    if server.returncode != 0:
        error = server.stderr.read().decode()
        raise AssertionError(f"server exited with {server.returncode}: {error}")


if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        print(error, file=sys.stderr)
        sys.exit(1)
