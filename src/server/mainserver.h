#include "clientconnection.h"
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

    ClientConnection **clients;
};

Server *server_new(int server_id);
void server_set_max_clients(Server *this, int max_clients);
void server_run(Server *this);

#endif
