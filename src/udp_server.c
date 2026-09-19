#include "udp_server.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#define UDP_REQUEST_SIZE 4096

int udp_server_run(uint16_t port, udp_request_handler handler, void *context,
                   volatile sig_atomic_t *running)
{
    struct sockaddr_in address;
    int server_fd;

    if (handler == NULL || running == NULL) {
        return UDP_SERVER_INVALID_ARGUMENT;
    }

    server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_fd < 0) {
        return UDP_SERVER_SOCKET_ERROR;
    }

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        close(server_fd);
        return UDP_SERVER_BIND_ERROR;
    }

    while (*running) {
        fd_set readable;
        struct timeval timeout = {0, 100000};
        int ready;

        FD_ZERO(&readable);
        FD_SET(server_fd, &readable);
        ready = select(server_fd + 1, &readable, NULL, NULL, &timeout);
        if (ready == 0) {
            continue;
        }
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            close(server_fd);
            return UDP_SERVER_IO_ERROR;
        }

        {
            char request[UDP_REQUEST_SIZE];
            struct sockaddr_in client;
            socklen_t client_length = sizeof(client);
            ssize_t received = recvfrom(server_fd, request, sizeof(request) - 1,
                                        MSG_TRUNC,
                                        (struct sockaddr *)&client,
                                        &client_length);
            char *response = NULL;
            int handler_result;

            if (received < 0) {
                if (errno == EINTR) {
                    continue;
                }
                close(server_fd);
                return UDP_SERVER_IO_ERROR;
            }
            if ((size_t)received >= sizeof(request)) {
                response = strdup("ERR request too long\n");
            } else {
                request[received] = '\0';
                while (received > 0 &&
                       (request[received - 1] == '\n' ||
                        request[received - 1] == '\r')) {
                    request[--received] = '\0';
                }
                handler_result = handler(request, &response, context);
                if (handler_result != 0 || response == NULL) {
                    free(response);
                    response = strdup("ERR internal error\n");
                }
            }
            if (response == NULL) {
                close(server_fd);
                return UDP_SERVER_IO_ERROR;
            }
            if (sendto(server_fd, response, strlen(response), 0,
                       (struct sockaddr *)&client, client_length) < 0) {
                free(response);
                close(server_fd);
                return UDP_SERVER_IO_ERROR;
            }
            free(response);
        }
    }

    close(server_fd);
    return UDP_SERVER_OK;
}
