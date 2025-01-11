#include <stdio.h>
#include <unistd.h>

#include "cserver.h"
#include "shared.h"

/*

*/
GServer *create_gserver(CServer *this, int id) {
    GServer *gserver = gserver_new(id);
    gserver->status = GSS_UNRESERVED;

    Server *internal_gserver = gserver->server;

    char gserver_name_buffer[MAX_GSERVER_NAME_CHARACTERS];
    snprintf(gserver_name_buffer, sizeof(gserver_name_buffer), "GameServer%d", id);
    strcpy(internal_gserver->name, gserver_name_buffer);

    char gserver_wkp_name_buffer[GSERVER_WKP_NAME_LEN];
    snprintf(gserver_wkp_name_buffer, sizeof(gserver_wkp_name_buffer), "G%d", id);
    strcpy(internal_gserver->wkp_name, gserver_wkp_name_buffer);
}

CServer *cserver_new(int id) {
    CServer *this = malloc(sizeof(CServer));

    this->gserver_count = MAX_CSERVER_GSERVERS;
    this->gserver_list = malloc(sizeof(GServer *) * this->gserver_count);
    this->server = server_new(id);

    strcpy(this->server->name, "CentralServer");
    strcpy(this->server->wkp_name, CSERVER_WKP_NAME); // WKP is "CSERVER"

    for (int i = 0; i < this->gserver_count; ++i) {
        this->gserver_list[i] = create_gserver(this, i);
    }

    return this;
}

void reserve_gserver(CServer *this) {
    int gserver_id = -1;

    for (int i = 0; i < this->gserver_count; ++i) {
        GServer *gserver = this->gserver_list[i];

        if (gserver->status == GSS_UNRESERVED) {
            gserver_id = i;
            break;
        }
    }

        // Get available GServer
}

void cserver_handle_net_event(CServer *this, int client_id, NetEvent *event) {
    void *args = event->args;

    switch (event->protocol) {

    default:
        break;
    }
}

void cserver_loop(CServer *this) {
    Server *server = this->server;

    for (int client_id = 0; client_id < server->max_clients; ++client_id) {
        Client *client = server->clients[client_id];
        if (client->is_free) {
            continue;
        }

        NetEventQueue *queue = client->recv_queue;
        for (int i = 0; i < queue->event_count; ++i) {
            cserver_handle_net_event(this, client_id, queue->events[i]);
        }
    }
}

void cserver_run(CServer *this) {
    Server *server = this->server;

    server_start_connection_handler(server);

    while (1) {
        handle_connections(server);

        server_empty_recv_events(server);
        server_recv_events(server);

        cserver_loop(this);

        server_send_events(server);
        usleep(TICK_TIME_MICROSECONDS);
    }
}