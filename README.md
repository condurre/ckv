# ckv

`ckv` is a small networkable, in-memory key-value store written in C. The
storage API and transports are independent: TCP and UDP both call the same
`kv_store` API without depending on each other.

## Security warning

`ckv` is an educational example and is **not a secure production datastore**.
It provides no authentication, authorization, encryption, or protection
against denial-of-service attacks. Do not expose it to the public internet or
use it to store sensitive data. If you run it, keep it on a trusted private
network or bind it behind an appropriately configured firewall and secure
proxy. UDP is also unauthenticated and unencrypted, and has the same security
exposure as TCP because both transports share the configured port.

## Build

```sh
make
make test
```

The server listens on all interfaces on the same numeric port for both TCP and
UDP, using port `6379` by default:

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

TCP uses newline-delimited streams. UDP treats each datagram as exactly one
request and sends exactly one response datagram; a trailing newline is
optional. Oversized UDP datagrams receive `ERR request too long`.

The transport modules handle sockets, framing, and I/O only. Request
interpretation is supplied through a callback, while `kv_store` owns key-value
storage. TCP accepted clients are handled by detached worker threads, so slow
or idle connections do not block other clients. UDP and TCP share the same
store, so mutations are visible across both protocols.