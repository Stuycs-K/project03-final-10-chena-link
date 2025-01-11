#include <stdio.h>
#include <unistd.h>

#include "cserver.h"
#include "shared.h"

CServer *cserver_new(int id) {
    CServer *this = malloc(sizeof(CServer));

    this->gserver_count = MAX_CSERVER_GSERVERS;
    this->gserver_list = malloc(sizeof(GServer *) * this->gserver_count);
    this->server = server_new(id);

    strcpy(this->server->name, "CentralServer");
    strcpy(this->server->wkp_name, CSERVER_WKP_NAME); // WKP is "CSERVER"

    for (int i = 0; i < this->gserver_count; ++i) {
        this->gserver_list[i] = gserver_new(id);
        this->gserver_list[i]->status = GSS_UNRESERVED;
    }

    return this;
}

void reserve_gserver(CServer *this) {
    GServer *gserver = NULL;

    // Get unreserved GServer
    for (int i = 0; i < this->gserver_count; ++i) {
        if (this->gserver_list[i]->status == GSS_UNRESERVED) {
            gserver = this->gserver_list[i];
            break;
        }
    }

    if (!gserver) {
        return;
    }

    gserver->status = GSS_WAITING_FOR_PLAYERS;

    pipe(gserver->cserver_pipes);
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