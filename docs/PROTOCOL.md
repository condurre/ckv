# ckv protocol

`ckv` exposes the same in-memory key-value store over TCP and UDP. The
default port is `6379`; passing a port to `./ckv [port]` changes both
listeners. Both sockets bind to all interfaces (`0.0.0.0`) and share one
store, so a mutation made over one transport is immediately visible over the
other.

## Requests and responses

Commands are ASCII, separated by whitespace:

```text
SET <key> <value>
GET <key>
DEL <key>
QUIT
```

`SET` stores the remainder after the key as the value, including internal
spaces. Leading whitespace before the value is treated as the separator.
Keys are limited to 1023 characters and values to 3071 characters by the
request parser. Keys and values cannot contain a newline because newline
terminates a request. Command names are case-sensitive.

Every response is terminated by `\n`:

| Request | Response |
| --- | --- |
| `SET key value` | `OK\n` |
| `GET key` for an existing key | `<value>\n` |
| `GET key` for a missing key | `NOT_FOUND\n` |
| `DEL key` for an existing key | `OK\n` |
| `DEL key` for a missing key | `NOT_FOUND\n` |
| `QUIT` | `BYE\n` |

An empty request returns `ERR empty request\n`. Any command with the wrong
number of arguments, an unknown command, or an otherwise invalid shape returns
`ERR usage: SET key value | GET key | DEL key | QUIT\n`. Allocation or
transport failures can return `ERR out of memory\n`, `ERR internal error\n`, or
`ERR\n`.

`QUIT` acknowledges the request but does not itself cause the server to close
the connection; clients should close the connection after reading `BYE\n`.

## TCP transport

TCP is a byte stream. Send one request ending in `\n` and read one complete
newline-terminated response. A connection may carry multiple requests, and
several clients may connect concurrently. A final `\r` immediately before
`\n` is ignored, so CRLF framing is accepted.

Requests longer than 4095 bytes before their newline receive
`ERR request too long\n`. The server then resets its request buffer and
continues reading that connection.

## UDP transport

Each UDP datagram is exactly one request, and the server sends exactly one
response datagram to the sender. A request datagram may omit a trailing
newline; trailing `\n` and `\r` characters are removed before parsing. The
response still ends in `\n`.

Datagrams of 4096 bytes or more receive `ERR request too long\n`. UDP does not
provide delivery, ordering, or duplicate protection: requests or responses
can be lost, duplicated, or reordered. Applications that need reliable
operations must add timeouts, retries, and an idempotency strategy, or use
TCP.

## Security and deployment

The server has no authentication, authorization, encryption, access control,
or denial-of-service protection. TCP and UDP are both unauthenticated and
unencrypted, and both listeners are exposed on all interfaces by default.
Do not expose `ckv` to the public internet or use it for sensitive data.
Keep it on a trusted private network or place it behind an appropriately
configured firewall and secure proxy. UDP's reliability limitations do not
make it safer than TCP.

Runnable examples are in [`examples/tcp_client.py`](../examples/tcp_client.py)
and [`examples/udp_client.py`](../examples/udp_client.py).
