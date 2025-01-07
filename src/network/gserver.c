#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "gserver.h"
#include "pipehandshake.h"

// GSubserver handles communication with ONE client.
typedef struct GSubserver GSubserver;
struct GSubserver {
    pid_t pid;
    int client_id;
    int send_fd;
    int recv_fd;
};

GSubserver *gsubserver_new(int recv_fd) {
    GSubserver *gsubserver = malloc(sizeof(GSubserver));

    gsubserver->client_id = -1;
    gsubserver->send_fd = -1;
    gsubserver->recv_fd = recv_fd;
    gsubserver->pid = -1;

    return gsubserver;
}

void gsubserver_init(GSubserver *gsubserver) {
    int send_fd = server_handshake(gsubserver->recv_fd);

    printf("CONNECTION MADE WITH CLIENT!\n");

    exit(EXIT_SUCCESS);
}

char *get_client_to_server_fifo_name() {
    // return getpid();
    return "TEMP";
}

GServer *gserver_new() {
    GServer *gserver = malloc(sizeof(GServer));

    gserver->max_clients = 2;
    gserver->current_clients = 0;
    gserver->id = 0;

    gserver->name = NULL;

    return gserver;
}

void gserver_init(GServer *gserver) {
    int from_client;
    int to_client;

    while (1) {
        from_client = server_setup(get_client_to_server_fifo_name());
        GSubserver *subserver = gsubserver_new(from_client);

        pid_t pid = fork();

        if (pid == 0) {
            gsubserver_init(subserver);
        } else {
        }
    }
}