#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "gserver.h"
#include "network/pipehandshake.h"
#include "network/pipenet.h"
#include "network/pipenetevents.h"
#include "shared.h"

static void handle_sigint(int signo) {
    if (signo != SIGINT) {
        return;
    }

    remove("TEMP");
    exit(EXIT_SUCCESS);
}

GServer *gserver_new(int id) {
    GServer *this = malloc(sizeof(GServer));
    this->status = GSS_WAITING_FOR_PLAYERS;

    this->server = server_new(id);

    return this;
}

// HANDLE CLIENT EVENTS (i.e. change game state) HERE
void gserver_handle_net_event(GServer *this, int client_id, NetEvent *event) {
    void *args = event->args;

    switch (event->protocol) {

    default:
        break;
    }
}

// ALL GAME LOGIC GOES HERE
void gserver_loop(GServer *this) {
    Server *server = this->server;

    for (int client_id = 0; client_id < server->max_clients; ++client_id) {
        Client *client = server->clients[client_id];
        if (client->is_free) {
            continue;
        }

        NetEventQueue *queue = client->recv_queue;
        for (int i = 0; i < queue->event_count; ++i) {
            gserver_handle_net_event(this, client_id, queue->events[i]);
        }
    }

    // BELOW IS A TEST FOR SERVER SEND
    NetArgs_PeriodicHandshake *test_args = malloc(sizeof(NetArgs_PeriodicHandshake));
    test_args->id = rand();

    printf("rand: %d\n", test_args->id);
    NetEvent *test_event = net_event_new(PERIODIC_HANDSHAKE, test_args);

    server_send_event_to_all(server, test_event);
}

void gserver_run(GServer *this) {
    signal(SIGINT, handle_sigint);
    Server *server = this->server;

    server_start_connection_handler(server);

    while (1) {
        handle_connections(server);
        server_recv_events(server);

        gserver_loop(this);

        server_empty_recv_events(server);

        server_send_events(server);
        usleep(TICK_TIME_MICROSECONDS);
    }
}
