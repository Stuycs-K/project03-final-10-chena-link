#include <stdio.h>
<<<<<<< HEAD
#include <unistd.h>

=======
<<<<<<< HEAD
=======

>>>>>>> e7867147ddb8cf24ae7f5f6aa8bac133f1415003
#include "clientconnection.h"
>>>>>>> 80b08123f68df61ed682db03f9c0500769fc2aef
#include "../network/pipehandshake.h"
#include "../network/pipenetevents.h"
#include "../shared.h"
#include "clientconnection.h"

Client *client_connection_new(int id) {
    Client *this = malloc(sizeof(Client));

    this->id = id;
    this->send_fd = -1;
    this->recv_fd = -1;
    this->recently_connected = 0;
    this->is_free = CONNECTION_IS_FREE;

    this->send_queue = net_event_queue_new();
    this->recv_queue = net_event_queue_new();

    return this;
}

void disconnect_client(Client *this) {
    this->is_free = CONNECTION_IS_FREE;

    close(this->send_fd);
    close(this->recv_fd);

    this->send_fd = -1;
    this->recv_fd = -1;

    empty_net_event_queue(this->send_queue);
    empty_net_event_queue(this->recv_queue);
}

void free_client_connection(Client *this) {
    disconnect_client(this);

    free(this->send_queue);
    free(this->recv_queue);
    free(this);
}
