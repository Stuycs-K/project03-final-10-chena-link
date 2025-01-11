#include "../network/pipenet.h"
#include "subserver.h"

#ifndef CLIENTCONNECTION_H
#define CLIENTCONNECTION_H

typedef struct Client Client;
struct Client {
    int id;
    char *name;

    int is_free;

    int send_fd;
    int recv_fd;

    NetEventQueue *send_queue;
    NetEventQueue *recv_queue;
};

Client *client_connection_new(int id);
void disconnect_client(Client *this);
void free_client_connection(Client *this);

#endif