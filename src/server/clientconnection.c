#include <stdio.h>
#include <unistd.h>

#include "../network/pipehandshake.h"
#include "../network/pipenetevents.h"
#include "../shared.h"
#include "clientconnection.h"

Client *client_connection_new(int id) {
    Client *this = malloc(sizeof(Client));

    this->id = id;
    this->name = calloc(sizeof(char), MAX_PLAYER_NAME_CHARACTERS);
    this->send_fd = -1;
    this->recv_fd = -1;

    this->is_free = CONNECTION_IS_FREE;

    this->send_queue = net_event_queue_new();
    this->recv_queue = net_event_queue_new();

    return this;
}

void disconnect_client(Client *this) {
    this->is_free = CONNECTION_IS_FREE;
    memset(this->name, 0, MAX_PLAYER_NAME_CHARACTERS);

    close(this->send_fd);
    close(this->recv_fd);

    this->send_fd = -1;
    this->recv_fd = -1;

    empty_net_event_queue(this->send_queue);
    empty_net_event_queue(this->recv_queue);
}

void free_client_connection(Client *this) {
    free(this->name);
    free(this->send_queue);
    free(this->recv_queue);
    free(this);
}
