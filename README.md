# ckv

`ckv` is a small networkable, in-memory key-value store written in C. The
storage API and TCP transport are independent: a future transport can call the
same `kv_store` API without depending on the TCP implementation.

## Security warning

`ckv` is an educational example and is **not a secure production datastore**.
It provides no authentication, authorization, encryption, or protection
against denial-of-service attacks. Do not expose it to the public internet or
use it to store sensitive data. If you run it, keep it on a trusted private
network or bind it behind an appropriately configured firewall and secure
proxy.

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
is supplied through a callback, while `kv_store` owns key-value storage.