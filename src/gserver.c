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

GServer *gserver_new(int id) {
    GServer *this = malloc(sizeof(GServer));
    this->status = GSS_WAITING_FOR_PLAYERS;

    this->server = server_new(id);
    this->server->max_clients = DEFAULT_GSERVER_MAX_CLIENTS;

    char gserver_name_buffer[MAX_GSERVER_NAME_CHARACTERS];
    snprintf(gserver_name_buffer, sizeof(gserver_name_buffer), "GameServer%d", id);
    strcpy(this->server->name, gserver_name_buffer);

    char gserver_wkp_name_buffer[GSERVER_WKP_NAME_LEN];
    snprintf(gserver_wkp_name_buffer, sizeof(gserver_wkp_name_buffer), "G%d", id);
    strcpy(this->server->wkp_name, gserver_wkp_name_buffer);

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

    FOREACH_CLIENT(server) {
        NetEventQueue *queue = client->recv_queue;
        for (int i = 0; i < queue->event_count; ++i) {
            gserver_handle_net_event(this, client_id, queue->events[i]);
        }
    }
    END_FOREACH_CLIENT()

    /* BELOW IS A TEST FOR SERVER SEND
    NetArgs_PeriodicHandshake *test_args = malloc(sizeof(NetArgs_PeriodicHandshake));
    test_args->id = rand();

    printf("rand: %d\n", test_args->id);
    NetEvent *test_event = net_event_new(PERIODIC_HANDSHAKE, test_args);

    server_send_event_to_all(server, test_event);
    */
}

void gserver_run(GServer *this) {
    Server *server = this->server;

    server_start_connection_handler(server);

    while (1) {
        handle_connections(server);

        server_empty_recv_events(server);
        server_recv_events(server);

        gserver_loop(this);

        server_send_events(server);
        usleep(TICK_TIME_MICROSECONDS);
    }
}
