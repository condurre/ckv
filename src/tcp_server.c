#include "tcp_server.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

#define TCP_REQUEST_SIZE 4096
#define TCP_BACKLOG 16

static int send_all(int client_fd, const char *data, size_t length)
{
    size_t sent = 0;

    while (sent < length) {
        ssize_t written = send(client_fd, data + sent, length - sent, 0);

        if (written <= 0) {
            return TCP_SERVER_IO_ERROR;
        }
        sent += (size_t)written;
    }

    return TCP_SERVER_OK;
}

static int serve_client(int client_fd, tcp_request_handler handler, void *context)
{
    char request[TCP_REQUEST_SIZE];
    size_t used = 0;

    for (;;) {
        char character;
        ssize_t received = recv(client_fd, &character, 1, 0);

        if (received == 0) {
            return TCP_SERVER_OK;
        }
        if (received < 0) {
            if (errno == EINTR) {
                continue;
            }
            return TCP_SERVER_IO_ERROR;
        }
        if (character == '\n') {
            char *response = NULL;
            int handler_result;
            int send_result;

            request[used] = '\0';
            if (used > 0 && request[used - 1] == '\r') {
                request[used - 1] = '\0';
            }

            handler_result = handler(request, &response, context);
            if (handler_result != 0 || response == NULL) {
                response = strdup("ERR internal error\n");
            }
            if (response == NULL) {
                return TCP_SERVER_IO_ERROR;
            }
            send_result = send_all(client_fd, response, strlen(response));
            free(response);
            if (send_result != TCP_SERVER_OK) {
                return send_result;
            }
            used = 0;
            continue;
        }
        if (used + 1 >= sizeof(request)) {
            int send_result = send_all(client_fd, "ERR request too long\n", 21);

            used = 0;
            if (send_result != TCP_SERVER_OK) {
                return send_result;
            }
            continue;
        }

        request[used] = character;
        used++;
    }
}

struct client_context {
    int client_fd;
    tcp_request_handler handler;
    void *handler_context;
};

static void *serve_client_thread(void *argument)
{
    struct client_context *client = argument;

    serve_client(client->client_fd, client->handler, client->handler_context);
    close(client->client_fd);
    free(client);
    return NULL;
}

static volatile sig_atomic_t *default_running;

static void stop_default_server(int signal_number)
{
    (void)signal_number;
    *default_running = 0;
}

int tcp_server_run_until_stopped(uint16_t port, tcp_request_handler handler,
                                 void *context, volatile sig_atomic_t *running)
{
    struct sockaddr_in address;
    int server_fd;
    int option = 1;
    int option_result;
    int bind_result;
    int listen_result;

    if (handler == NULL || running == NULL) {
        return TCP_SERVER_INVALID_ARGUMENT;
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        return TCP_SERVER_SOCKET_ERROR;
    }

    option_result = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    if (option_result < 0) {
        close(server_fd);
        return TCP_SERVER_SOCKET_ERROR;
    }
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);

    bind_result = bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    if (bind_result < 0) {
        close(server_fd);
        return TCP_SERVER_BIND_ERROR;
    }

    listen_result = listen(server_fd, TCP_BACKLOG);
    if (listen_result < 0) {
        close(server_fd);
        return TCP_SERVER_LISTEN_ERROR;
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
            return TCP_SERVER_ACCEPT_ERROR;
        }

        int client_fd = accept(server_fd, NULL, NULL);

        if (client_fd < 0) {
            if (errno == EINTR && !*running) {
                break;
            }
            if (errno == EINTR) {
                continue;
            }
            close(server_fd);
            return TCP_SERVER_ACCEPT_ERROR;
        }

        struct client_context *client = malloc(sizeof(*client));
        pthread_t thread;
        int thread_result;
        int detach_result;

        if (client == NULL) {
            fprintf(stderr, "failed to allocate client context\n");
            close(client_fd);
            continue;
        }

        client->client_fd = client_fd;
        client->handler = handler;
        client->handler_context = context;
        thread_result = pthread_create(&thread, NULL, serve_client_thread, client);
        if (thread_result != 0) {
            fprintf(stderr, "failed to create client thread: %s\n",
                    strerror(thread_result));
            free(client);
            close(client_fd);
            continue;
        }

        detach_result = pthread_detach(thread);
        if (detach_result != 0) {
            fprintf(stderr, "failed to detach client thread: %s\n",
                    strerror(detach_result));
            if (pthread_join(thread, NULL) != 0) {
                fprintf(stderr, "failed to join client thread after detach failure\n");
            }
        }
    }

    close(server_fd);
    return TCP_SERVER_OK;
}

int tcp_server_run(uint16_t port, tcp_request_handler handler, void *context)
{
    static volatile sig_atomic_t running = 1;
    struct sigaction action;

    memset(&action, 0, sizeof(action));
    default_running = &running;
    action.sa_handler = stop_default_server;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGINT, &action, NULL) < 0 ||
        sigaction(SIGTERM, &action, NULL) < 0) {
        return TCP_SERVER_SIGNAL_ERROR;
    }
    return tcp_server_run_until_stopped(port, handler, context, &running);
}
