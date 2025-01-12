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

void update_gserver_info(GServer *this) {
    GServerInfo *server_info = this->info_event->args;

    // These are the only fields that will ever change
    server_info->current_clients = this->server->current_clients;
    server_info->max_clients = this->server->max_clients;
    server_info->status = this->status;
    strcpy(server_info->name, this->server->name);

    // int to_cserver_fd = this->cserver_pipes[PIPE_WRITE];
}

/*
    If any clients recently joined, update our GServerInfo and send it
    to the CServer so they can update their information about us and inform
    all clients.
*/
void check_update_gserver_info(GServer *this) {
    Server *server = this->server;

    int did_client_list_change = 0;
    FOREACH_CLIENT(server) {
        if (client->recently_connected || client->recently_disconnected) {
            did_client_list_change = 1;
            update_gserver_info(this);

            break;
        }
    }
    END_FOREACH_CLIENT()

    if (did_client_list_change) {
        attach_event(this->cserver_send_queue, this->info_event);
    } else {
        detach_event(this->cserver_send_queue, this->info_event);
    }
}

/*
    Send any events to the CServer
*/
void send_to_cserver(GServer *this) {
    send_event_queue(this->cserver_send_queue, this->cserver_pipes[PIPE_WRITE]);
}

GServer *gserver_new(int id) {
    GServer *this = malloc(sizeof(GServer));

    GServerInfo *server_info = nargs_gserver_info();
    this->info_event = net_event_new(GSERVER_INFO, server_info);

    this->cserver_send_queue = net_event_queue_new();

    this->status = GSS_WAITING_FOR_PLAYERS;

    this->server = server_new(id);
    this->server->max_clients = DEFAULT_GSERVER_MAX_CLIENTS;

    char gserver_name_buffer[MAX_GSERVER_NAME_CHARACTERS];
    snprintf(gserver_name_buffer, sizeof(gserver_name_buffer), "GameServer%d", id);
    strcpy(this->server->name, gserver_name_buffer);

    char gserver_wkp_name_buffer[GSERVER_WKP_NAME_LEN];
    snprintf(gserver_wkp_name_buffer, sizeof(gserver_wkp_name_buffer), "G%d", id);
    strcpy(this->server->wkp_name, gserver_wkp_name_buffer);

    // Update GServerInfo
    server_info->id = id;
    strcpy(server_info->wkp_name, gserver_wkp_name_buffer);

    update_gserver_info(this);

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

    check_update_gserver_info(this);

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

    // Last
    send_to_cserver(this);
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
