#include "kv_store.h"
#include "tcp_server.h"
#include "udp_server.h"

#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STORE_BUCKETS 1024

struct server_thread_context {
    unsigned short port;
    kv_store *store;
    volatile sig_atomic_t *running;
    int result;
    int is_udp;
};

volatile sig_atomic_t servers_running = 1;
static int handle_request(const char *request, char **response, void *context);

static void stop_servers(int signal_number)
{
    (void)signal_number;
    servers_running = 0;
}

static void *run_server(void *argument)
{
    struct server_thread_context *server = argument;

    if (server->is_udp) {
        server->result = udp_server_run(server->port, handle_request,
                                        server->store, server->running);
    } else {
        server->result = tcp_server_run_until_stopped(
            server->port, handle_request, server->store, server->running);
    }
    if (server->result != 0) {
        *server->running = 0;
    }
    return NULL;
}

static char *response_for(const char *text)
{
    size_t length = strlen(text);
    char *response = malloc(length + 2);

    if (response == NULL) {
        return NULL;
    }

    memcpy(response, text, length);
    response[length] = '\n';
    response[length + 1] = '\0';
    return response;
}

static int handle_request(const char *request, char **response, void *context)
{
    kv_store *store = context;
    char command[5];
    char key[1024];
    char value[3072];
    int fields;

    fields = sscanf(request, "%4s %1023s %3071[^\n]", command, key, value);
    if (fields < 1) {
        *response = response_for("ERR empty request");
        return *response == NULL;
    }

    if (strcmp(command, "SET") == 0 && fields == 3) {
        int result = kv_store_set(store, key, value);

        *response = response_for(result == KV_STORE_OK ? "OK" : "ERR out of memory");
        return *response == NULL;
    }

    if (strcmp(command, "GET") == 0 && fields == 2) {
        char *stored_value = NULL;
        int result = kv_store_get(store, key, &stored_value);

        if (result == KV_STORE_OK) {
            *response = response_for(stored_value);
            kv_store_free_value(stored_value);
        } else {
            *response = response_for(result == KV_STORE_NOT_FOUND ? "NOT_FOUND" : "ERR");
        }
        return *response == NULL;
    }

    if (strcmp(command, "DEL") == 0 && fields == 2) {
        int result = kv_store_delete(store, key);

        *response = response_for(result == KV_STORE_OK ? "OK" :
                                 result == KV_STORE_NOT_FOUND ? "NOT_FOUND" : "ERR");
        return *response == NULL;
    }

    if (strcmp(command, "QUIT") == 0 && fields == 1) {
        *response = response_for("BYE");
        return *response == NULL;
    }

    *response = response_for("ERR usage: SET key value | GET key | DEL key | QUIT");
    return *response == NULL;
}

int main(int argc, char **argv)
{
    kv_store *store;
    unsigned long port = 6379;
    char *end = NULL;
    struct sigaction action;
    struct server_thread_context tcp_context;
    struct server_thread_context udp_context;
    pthread_t tcp_thread;
    pthread_t udp_thread;
    int server_result;

    if (argc > 1) {
        port = strtoul(argv[1], &end, 10);
        if (*argv[1] == '\0' || *end != '\0' || port > 65535) {
            fprintf(stderr, "usage: %s [port]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }

    store = kv_store_create(STORE_BUCKETS);
    if (store == NULL) {
        fprintf(stderr, "failed to create key-value store\n");
        return EXIT_FAILURE;
    }

    memset(&action, 0, sizeof(action));
    action.sa_handler = stop_servers;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGINT, &action, NULL) < 0 ||
        sigaction(SIGTERM, &action, NULL) < 0) {
        fprintf(stderr, "failed to install signal handlers: %s\n",
                strerror(errno));
        kv_store_destroy(store);
        return EXIT_FAILURE;
    }

    tcp_context = (struct server_thread_context){
        (unsigned short)port, store, &servers_running, 0, 0
    };
    udp_context = (struct server_thread_context){
        (unsigned short)port, store, &servers_running, 0, 1
    };
    printf("ckv listening on TCP and UDP port %lu\n", port);
    if (pthread_create(&tcp_thread, NULL, run_server, &tcp_context) != 0) {
        fprintf(stderr, "failed to start server threads\n");
        servers_running = 0;
        kv_store_destroy(store);
        return EXIT_FAILURE;
    }
    if (pthread_create(&udp_thread, NULL, run_server, &udp_context) != 0) {
        fprintf(stderr, "failed to start server threads\n");
        servers_running = 0;
        pthread_join(tcp_thread, NULL);
        kv_store_destroy(store);
        return EXIT_FAILURE;
    }
    pthread_join(tcp_thread, NULL);
    servers_running = 0;
    pthread_join(udp_thread, NULL);
    server_result = tcp_context.result != 0 ? tcp_context.result :
                    udp_context.result;
    kv_store_destroy(store);

    if (server_result != TCP_SERVER_OK) {
        fprintf(stderr, "server error: %d\n", server_result);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
