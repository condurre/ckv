#!/usr/bin/env python3

import socket
import subprocess
import sys
import threading
import time


def request(port, text):
    with socket.create_connection(("127.0.0.1", port), timeout=2) as client:
        client.sendall((text + "\n").encode())
        response = client.recv(4096).decode()
        if response != "OK\n":
            raise AssertionError(f"unexpected response: {response!r}")


def main():
    with socket.socket() as probe:
        probe.bind(("127.0.0.1", 0))
        port = probe.getsockname()[1]
    server = subprocess.Popen(
        ["./ckv", str(port)],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
    )
    blocker = None

    try:
        deadline = time.time() + 2
        while time.time() < deadline:
            try:
                blocker = socket.create_connection(("127.0.0.1", port), timeout=1)
                break
            except OSError:
                time.sleep(0.01)
        if blocker is None:
            raise AssertionError("server did not accept connections")

        errors = []

        def run_request(index):
            try:
                request(port, f"SET key-{index} value")
            except Exception as error:
                errors.append(error)

        clients = [threading.Thread(target=run_request, args=(index,))
                   for index in range(8)]
        for client in clients:
            client.start()
        for client in clients:
            client.join(timeout=2)
            if client.is_alive():
                raise AssertionError("concurrent client did not complete")
        if errors:
            raise AssertionError(f"concurrent client failed: {errors[0]}")

        blocker.close()
        blocker = None
    finally:
        if blocker is not None:
            blocker.close()
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
