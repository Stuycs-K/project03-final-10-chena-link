#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "gserver.h"
#include "pipehandshake.h"
#include "pipenet.h"
#include "pipenetevents.h"

GSubserver *gsubserver_new(int client_id) {
    GSubserver *gsubserver = malloc(sizeof(GSubserver));

    gsubserver->client_id = client_id;
    gsubserver->send_fd = -1;
    gsubserver->recv_fd = -1;
    gsubserver->pid = -1;

    gsubserver->handshake_event = NULL;

    return gsubserver;
}

/*
    Finishes the server handshake.
    Then, receives all client network signals.
*/
void gsubserver_init(GSubserver *gsubserver) {
    NetEventQueue *net_recv_queue = net_event_queue_new();

    gsubserver->pid = getpid();

    int status = server_complete_handshake(gsubserver->handshake_event);

    free_handshake_event(gsubserver->handshake_event);
    gsubserver->handshake_event = NULL;

    if (status == -1) {
        printf("ACK Fail\n");
        exit(EXIT_FAILURE);
    }

    printf("CONNECTION MADE WITH CLIENT!\n");

    while (1) {
        empty_net_event_queue(net_recv_queue);

        size_t packet_size;
        ssize_t bytes_read = read(gsubserver->recv_fd, &packet_size, sizeof(packet_size));
        if (bytes_read <= 0) {
            if (bytes_read == 0) {
                // CLIENT DISCONNECT!
                break;
            }
        }

        char *raw_recv_buffer = malloc(sizeof(char) * packet_size);
        bytes_read = read(gsubserver->recv_fd, raw_recv_buffer, packet_size);

        if (bytes_read <= 0) {
            if (bytes_read == 0) {
                // CLIENT DISCONNECT!
                break;
            }
        }

        recv_event_queue(net_recv_queue, raw_recv_buffer);

        printf("ID: %d | Count: %d | 1Protocol: %d\n", gsubserver->client_id, net_recv_queue->event_count, net_recv_queue->events[0]->protocol);

        for (int i = 0; i < net_recv_queue->event_count; ++i) {
            NetEvent *event = net_recv_queue->events[i];
            NetArgs_PeriodicHandshake *nargs = (NetArgs_PeriodicHandshake *)event->args;
            printf("n: %d\n", nargs->id);
        }

        usleep(1000000);
    }

    printf("CLIENT DISCONNECT\n");

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

    gserver->status = GSS_WAITING_FOR_PLAYERS;
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

void gserver_set_max_clients(GServer *gserver, int max_clients) {
    int old_max_clients = gserver->max_clients;

    // Don't do anything if we're not changing anything
    if (old_max_clients == max_clients) {
        return;
    }

    // Can't make server accomodate less players than it already has
    if (gserver->current_clients > max_clients) {
        return;
    }

    int is_expanding = max_clients > old_max_clients;

    if (is_expanding) {
        gserver->subservers = realloc(gserver->subservers, sizeof(GSubserver *) * max_clients);

        for (int i = old_max_clients - 1; i < max_clients; ++i) {
            gserver->subservers[i] = gsubserver_new(i);
        }
    } else {
        for (int i = old_max_clients - 1; i >= max_clients - 1; --i) {
            free(gserver->subservers[i]);
        }

        gserver->subservers = realloc(gserver->subservers, sizeof(GSubserver *) * max_clients);
    }
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

GSubserver *gserver_handle_connection(GServer *gserver, NetEvent *handshake_event) {
    NetArgs_InitialHandshake *handshake = handshake_event->args;

    int client_id = gserver_get_free_client_id(gserver);
    handshake->client_id = client_id;

    GSubserver *chosen_subserver = gserver->subservers[client_id];

    chosen_subserver->recv_fd = handshake->client_to_server_fd;
    chosen_subserver->send_fd = handshake->server_to_client_fd;
    chosen_subserver->client_id = client_id;
    chosen_subserver->handshake_event = handshake_event;

    gserver->current_clients++;

    return chosen_subserver;
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

    while (1) {
        int from_client = server_setup(get_client_to_server_fifo_name());

        NetEvent *handshake_event = create_handshake_event();
        ((NetArgs_InitialHandshake *)handshake_event->args)->client_to_server_fd = from_client;

        server_get_send_fd(handshake_event);

        // A client connected, but the server is full!
        if (gserver->current_clients >= gserver->max_clients) {
            server_abort_handshake(handshake_event, HEC_SERVER_IS_FULL);
            free_handshake_event(handshake_event);

            printf("server is full!\n");

            continue;
        }

        GSubserver *subserver = gserver_handle_connection(gserver, handshake_event);
        if (subserver == NULL) {
            continue;
        }

        pid_t pid = fork();

        if (pid == 0) {
            gsubserver_init(subserver);
        } else {
            gserver->subservers[subserver->client_id]->pid = pid;
        }
    }
}
