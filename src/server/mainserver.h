#include "subserver.h"

#ifndef MAINSERVER_H
#define MAINSERVER_H

typedef struct Server Server;
struct Server {
    int max_clients;
    int current_clients;

    int id; // Given by the central server

    int subserver_pipe[2];

    char *name;

    Subserver **subservers; // List of subserver connections
};

Server *server_new(int server_id);
void server_set_max_clients(Server *this, int max_clients);
Subserver *server_setup_subserver_for_connection(Server *this, NetEvent *handshake_event);

#endif