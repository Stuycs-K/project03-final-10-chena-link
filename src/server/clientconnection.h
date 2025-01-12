#include "../network/pipenet.h"

#ifndef CLIENTCONNECTION_H
#define CLIENTCONNECTION_H

typedef struct Client Client;
struct Client {
    int id;
    int is_free;
    int recently_connected;    // Set to 1 upon a client joining, and 0 the next server tick.
    int recently_disconnected; // Set to 1 upon a client disconnecting, and 0 the next server tick.

    int send_fd;
    int recv_fd;

    NetEventQueue *send_queue;
    NetEventQueue *recv_queue;
};

Client *client_connection_new(int id);
void disconnect_client(Client *this);
void free_client_connection(Client *this);

#endif