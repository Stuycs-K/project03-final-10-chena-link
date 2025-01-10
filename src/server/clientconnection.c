#include "clientconnection.h"
#include "../shared.h"

ClientConnection *client_connection_new(int id) {
    ClientConnection *this = malloc(sizeof(ClientConnection));

    this->id = id;
    this->name = calloc(sizeof(char), MAX_PLAYER_NAME_CHARACTERS);
    this->send_fd = -1;

    this->send_queue = net_event_queue_new();
    this->recv_queue = net_event_queue_new();

    return this;
}

void disconnect_client(ClientConnection *this) {
}