#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "gserver.h"
#include "pipehandshake.h"

GSubserver *gsubserver_new(int client_id) {
    GSubserver *gsubserver = malloc(sizeof(GSubserver));

    gsubserver->client_id = client_id;
    gsubserver->send_fd = -1;
    gsubserver->recv_fd = -1;
    gsubserver->pid = -1;

    return gsubserver;
}

void gsubserver_init(GSubserver *gsubserver) {
    gsubserver->pid = getpid();

    int send_fd = server_handshake(gsubserver->recv_fd);
    gsubserver->send_fd = send_fd;

    printf("CONNECTION MADE WITH CLIENT!\n");

    while (1) {
    }

    exit(EXIT_SUCCESS);
}

int gsubserver_is_inactive(GSubserver *gsubserver) {
    return (gsubserver->pid == -1 && gsubserver->recv_fd == -1);
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

    // Populate with inactive subservers
    gserver->subservers = malloc(sizeof(GSubserver *) * gserver->max_clients);
    for (int i = 0; i < gserver->max_clients; ++i) {
        gserver->subservers[i] = gsubserver_new(i);
    }

    return gserver;
}

// -1 indicates the server is full
int gserver_get_free_client_id(GServer *gserver) {
    for (int i = 0; i < gserver->max_clients; ++i) {
        if (gsubserver_is_inactive(gserver->subservers[i])) {
            return i;
        }
    }

    return -1;
}

static void handle_sigint(int signo) {
    if (signo != SIGINT) {
        return;
    }

    remove(get_client_to_server_fifo_name());
    exit(EXIT_SUCCESS);
}

void gserver_init(GServer *gserver) {
    signal(SIGINT, handle_sigint);

    int from_client;
    int to_client;

    while (1) {
        from_client = server_setup(get_client_to_server_fifo_name());

        int client_id = gserver_get_free_client_id(gserver);

        // TODO: Send a kick message
        if (client_id == -1) {
            continue;
        }

        GSubserver *chosen_subserver = gserver->subservers[client_id];
        chosen_subserver->recv_fd = from_client;

        pid_t pid = fork();

        if (pid == 0) {
            gsubserver_init(chosen_subserver);
        } else {
        }
    }
}