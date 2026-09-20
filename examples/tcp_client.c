#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT "6379"
#define RESPONSE_SIZE 4096

static int parse_arguments(int argc, char **argv, const char **host,
                           const char **port)
{
    if (argc == 1) {
        *host = DEFAULT_HOST;
        *port = DEFAULT_PORT;
    } else if (argc == 2) {
        *host = DEFAULT_HOST;
        *port = argv[1];
    } else if (argc == 3) {
        *host = argv[1];
        *port = argv[2];
    } else {
        fprintf(stderr, "usage: %s [port] | %s [host] [port]\n",
                argv[0], argv[0]);
        return -1;
    }
    return 0;
}

static int connect_to_server(const char *host, const char *port)
{
    struct addrinfo hints;
    struct addrinfo *addresses = NULL;
    struct addrinfo *address;
    int socket_fd = -1;
    int result;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    result = getaddrinfo(host, port, &hints, &addresses);
    if (result != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(result));
        return -1;
    }

    for (address = addresses; address != NULL; address = address->ai_next) {
        int connect_result;

        socket_fd = socket(address->ai_family, address->ai_socktype,
                           address->ai_protocol);
        if (socket_fd < 0) {
            continue;
        }
        connect_result = connect(socket_fd, address->ai_addr,
                                 address->ai_addrlen);
        if (connect_result == 0) {
            break;
        }
        close(socket_fd);
        socket_fd = -1;
    }
    freeaddrinfo(addresses);

    if (socket_fd < 0) {
        fprintf(stderr, "could not connect to %s:%s: %s\n", host, port,
                strerror(errno));
    }
    return socket_fd;
}

static int send_all(int socket_fd, const char *request)
{
    size_t length = strlen(request);
    size_t sent = 0;

    while (sent < length) {
        ssize_t result = send(socket_fd, request + sent, length - sent, 0);

        if (result < 0 && errno == EINTR) {
            continue;
        }
        if (result <= 0) {
            return -1;
        }
        sent += (size_t)result;
    }
    return 0;
}

static int read_response(int socket_fd, char *response, size_t capacity)
{
    size_t used = 0;

    while (used + 1 < capacity) {
        char character;
        ssize_t result = recv(socket_fd, &character, 1, 0);

        if (result < 0 && errno == EINTR) {
            continue;
        }
        if (result <= 0) {
            return -1;
        }
        response[used++] = character;
        if (character == '\n') {
            response[used] = '\0';
            return 0;
        }
    }
    return -1;
}

static int request(int socket_fd, const char *command)
{
    char request_text[RESPONSE_SIZE];
    char response[RESPONSE_SIZE];
    int written;

    written = snprintf(request_text, sizeof(request_text), "%s\n", command);
    if (written < 0 || (size_t)written >= sizeof(request_text)) {
        fprintf(stderr, "request is too long: %s\n", command);
        return -1;
    }
    if (send_all(socket_fd, request_text) != 0) {
        fprintf(stderr, "send failed: %s\n", strerror(errno));
        return -1;
    }
    if (read_response(socket_fd, response, sizeof(response)) != 0) {
        fprintf(stderr, "read failed: %s\n", strerror(errno));
        return -1;
    }
    printf("%s -> %s", command, response);
    return 0;
}

int main(int argc, char **argv)
{
    const char *host;
    const char *port;
    int socket_fd;
    int result;

    result = parse_arguments(argc, argv, &host, &port);
    if (result != 0) {
        return EXIT_FAILURE;
    }
    socket_fd = connect_to_server(host, port);
    if (socket_fd < 0) {
        return EXIT_FAILURE;
    }

    result = request(socket_fd, "SET c_client_tcp hello from tcp");
    if (result == 0) {
        result = request(socket_fd, "GET c_client_tcp");
    }
    if (result == 0) {
        result = request(socket_fd, "DEL c_client_tcp");
    }
    if (result == 0) {
        result = request(socket_fd, "QUIT");
    }

    if (close(socket_fd) < 0) {
        fprintf(stderr, "close failed: %s\n", strerror(errno));
        result = -1;
    }
    return result == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
