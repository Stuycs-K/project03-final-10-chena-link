#include <stdio.h>
#include <unistd.h>

#include "../network/pipehandshake.h"
#include "../network/pipenetevents.h"
#include "../shared.h"
#include "clientconnection.h"

/*
    Create a new Client object.

    PARAMS:
        int id : which client this slot should indicate

    RETURNS: the new Client
*/
Client *client_connection_new(int id) {
    Client *this = malloc(sizeof(Client));

    this->id = id;
    this->send_fd = -1;
    this->recv_fd = -1;
    this->recently_connected = 0;
    this->is_free = 1;

    this->send_queue = net_event_queue_new();
    this->recv_queue = net_event_queue_new();

    return this;
}

/*
    Closes the connection's file descriptors and clears event queues.
    Note that this does not set is_free to true.

    PARAMS:
        Client *this : the connection to close

    RETURNS: none
*/
void disconnect_client(Client *this) {
    close(this->send_fd);
    close(this->recv_fd);

    this->send_fd = -1;
    this->recv_fd = -1;

    clear_event_queue(this->send_queue);
    clear_event_queue(this->recv_queue);
}

/*
    Frees the connection from memory

    PARAMS:
        Client *this : the connection to free

    RETURNS: none
*/
void free_client_connection(Client *this) {
    disconnect_client(this);

    free(this->send_queue);
    free(this->recv_queue);
    free(this);
}
