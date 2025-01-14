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

/*
    Updates the networked interface of the GServer to transmit to the CServer.
    Called whenever a player connects or disconnects.
    The only fields that will change are the server's status, current clients, maximum clients, and visible name

    PARAMS:
        GServer *this : the GServer

    RETURNS: none
*/
void update_gserver_info(GServer *this) {
    GServerInfo *server_info = this->info_event->args;

    // These are the only fields that will ever change
    server_info->current_clients = this->server->current_clients;
    server_info->max_clients = this->server->max_clients;
    server_info->status = this->status;
    strcpy(server_info->name, this->server->name);
}

/*
    If any clients recently joined or left, update our GServerInfo and send it
    to the CServer so they can update their information about us and inform
    all clients.

    PARAMS:
        GServer *this : the GServer

    RETURNS: none
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
    Send any events to the CServer. This will usually just be the GServerInfo.

    PARAMS:
        GServer *this : the GServer

    RETURNS: none
*/
void send_to_cserver(GServer *this) {
    send_event_queue(this->cserver_send_queue, this->cserver_pipes[PIPE_WRITE]);
    clear_event_queue(this->cserver_send_queue);
}

/*
    Creates a new GServer.
    The default name used is "GameServer" + id, and the WKP is "G" + id.

    PARAMS:
        int id : the GServer's ID. Given by the CServer.

    RETURNS: the new GServer
*/
GServer *gserver_new(int id) {
    GServer *this = malloc(sizeof(GServer));

    GServerInfo *server_info = nargs_gserver_info();
    this->info_event = net_event_new(GSERVER_INFO, server_info);

    this->cserver_send_queue = net_event_queue_new();
    this->cserver_recv_queue = net_event_queue_new();

    this->status = GSS_UNRESERVED;

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

/*
    Handles all client NetEvents (i.e. playing the game, starting the game)

    PARAMS:
        GServer *this : the GServer
        int client_id : which client
        NetEvent *event : the NetEvent this client sent us

    RETURNS: none
*/
void gserver_handle_net_event(GServer *this, int client_id, NetEvent *event) {
    void *args = event->args;

    switch (event->protocol) {

    default:
        break;
    }
}

/*
    The GServeer loop.
    1) Update GServerInfo.
    2) Processes client NetEvents.
    3) Update the game state and queues any NetEvents that needed to be sent back to clients.
    4) Send any events to the CServer.

    PARAMS:
        GServer *this : the GServer

    RETURNS: none
*/
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

    // Last
    send_to_cserver(this);
}

/*
    Starts the GServer. It will accept clients and be able to send / receive events.

    PARAMS:
        GServer *this : the GServer

    RETURNS: none
*/
void gserver_run(GServer *this) {
    Server *server = this->server;
    this->status = GSS_WAITING_FOR_PLAYERS;

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
