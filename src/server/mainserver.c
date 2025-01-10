#include <unistd.h>

#include "../network/pipenetevents.h"
#include "mainserver.h"

Server *server_new(int server_id) {
    Server *this = malloc(sizeof(Server));

    this->max_clients = 2;
    this->current_clients = 0;
    this->id = server_id;

    this->name = NULL;

    pipe(this->subserver_pipe);

    // Populate with inactive subservers
    this->subservers = malloc(sizeof(Subserver *) * this->max_clients);

    for (int i = 0; i < this->max_clients; ++i) {
        this->subservers[i] = subserver_new(i);

        // Set the subserver pipedes
        memcpy(this->subservers[i]->main_pipe, this->subserver_pipe, sizeof(this->subserver_pipe));
    }

    return this;
}

void server_set_max_clients(Server *this, int max_clients) {
    int old_max_clients = this->max_clients;

    // Don't do anything if we're not changing anything
    if (old_max_clients == max_clients) {
        return;
    }

    // Can't make server accomodate less players than it already has
    if (this->current_clients > max_clients) {
        return;
    }

    int is_expanding = max_clients > old_max_clients;

    if (is_expanding) {
        this->subservers = realloc(this->subservers, sizeof(Subserver *) * max_clients);

        for (int i = old_max_clients - 1; i < max_clients; ++i) {
            this->subservers[i] = subserver_new(i);
        }
    } else {
        for (int i = old_max_clients - 1; i >= max_clients - 1; --i) {
            free(this->subservers[i]);
        }

        this->subservers = realloc(this->subservers, sizeof(Subserver *) * max_clients);
    }
}

// -1 indicates the server is full
int server_get_free_client_id(Server *this) {
    for (int i = 0; i < this->max_clients; ++i) {
        if (subserver_is_inactive(this->subservers[i])) {
            return i;
        }
    }

    return -1;
}

Subserver *server_setup_subserver_for_connection(Server *this, NetEvent *handshake_event) {
    NetArgs_InitialHandshake *handshake = handshake_event->args;

    int client_id = server_get_free_client_id(this);
    handshake->client_id = client_id;

    Subserver *chosen_subserver = this->subservers[client_id];

    chosen_subserver->recv_fd = handshake->client_to_server_fd;
    chosen_subserver->send_fd = handshake->server_to_client_fd;
    chosen_subserver->client_id = client_id;
    chosen_subserver->handshake_event = handshake_event;

    this->current_clients++;

    return chosen_subserver;
}