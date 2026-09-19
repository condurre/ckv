#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <stdint.h>

typedef int (*tcp_request_handler)(const char *request, char **response, void *context);

enum tcp_server_result {
    TCP_SERVER_OK = 0,
    TCP_SERVER_INVALID_ARGUMENT = 1,
    TCP_SERVER_SOCKET_ERROR = 2,
    TCP_SERVER_BIND_ERROR = 3,
    TCP_SERVER_LISTEN_ERROR = 4,
    TCP_SERVER_ACCEPT_ERROR = 5,
    TCP_SERVER_IO_ERROR = 6,
    TCP_SERVER_SIGNAL_ERROR = 7
};

int tcp_server_run(uint16_t port, tcp_request_handler handler, void *context);

#endif
