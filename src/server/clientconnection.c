#include "clientconnection.h"

ClientConnection *client_connection_new(int id) {
    ClientConnection *this = malloc(sizeof(ClientConnection));

    this->id = id;
    this->name = calloc(sizeof(char), 20);
    this->send_fd = -1;
    this->subserver = subserver_new(id);

    return this;
}

void disconnect_client(ClientConnection *this) {
}