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

## Protocol and clients

The complete wire specification, limits, errors, transport behavior, and UDP
reliability caveats are in [`docs/PROTOCOL.md`](docs/PROTOCOL.md).

Start the server, then run either concise client example:

```sh
./ckv 6379
python3 examples/tcp_client.py 6379
python3 examples/udp_client.py 6379
```

Both clients print:

```text
SET example hello world -> OK
GET example -> hello world
DEL example -> OK
QUIT -> BYE
```

TCP accepts multiple newline-framed requests per connection. UDP uses one
datagram per request and response. Both transports use the same store and
port. The server binds all interfaces by default; see the security warning
above before running it outside a trusted private network.