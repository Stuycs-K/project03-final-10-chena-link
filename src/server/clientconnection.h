#include "../network/pipenet.h"

#ifndef CLIENTCONNECTION_H
#define CLIENTCONNECTION_H

typedef struct Client Client;
struct Client {
    int id;                    // An identifier used by the server
    int is_free;               // Can this struct hold a new connection?
    int recently_connected;    // Set to 1 upon a client joining, and 0 the next server tick.
    int recently_disconnected; // Set to 1 upon a client disconnecting, and 0 the next server tick.

    int send_fd; // The FD to which we'll send data
    int recv_fd; // THe FD from which we'll receive data

    NetEventQueue *send_queue; // NetEventQueue for sending
    NetEventQueue *recv_queue; // NetEventQueue for receiving
};

Client *client_connection_new(int id);
void disconnect_client(Client *this);
void free_client_connection(Client *this);

#endif