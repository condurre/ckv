#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include <signal.h>
#include <stdint.h>

typedef int (*udp_request_handler)(const char *request, char **response,
                                   void *context);

enum udp_server_result {
    UDP_SERVER_OK = 0,
    UDP_SERVER_INVALID_ARGUMENT = 1,
    UDP_SERVER_SOCKET_ERROR = 2,
    UDP_SERVER_BIND_ERROR = 3,
    UDP_SERVER_IO_ERROR = 4
};

int udp_server_run(uint16_t port, udp_request_handler handler, void *context,
                   volatile sig_atomic_t *running);

#endif
