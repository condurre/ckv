# ckv

`ckv` is a small networkable, in-memory key-value store written in C. The
storage API and TCP transport are independent: a future transport can call the
same `kv_store` API without depending on the TCP implementation.

## Build

```sh
make
make test
```

The server listens on all interfaces on port `6379` by default:

```sh
./ckv [port]
```

## Protocol

Requests are newline-delimited. Each client connection may send multiple
requests and receives one newline-delimited response for each request.

```text
SET key value  -> OK
GET key        -> the value, or NOT_FOUND
DEL key        -> OK, or NOT_FOUND
QUIT           -> BYE
```

The TCP server handles sockets, framing, and I/O only. Request interpretation
is supplied through a callback, while `kv_store` owns key-value storage. Each
accepted client is handled by a detached worker thread, so slow or idle
connections do not block other clients.