/*
    GServer, or game server handles players for a single game instance.
*/

#ifndef GSERVER_H
#define GSERVER_H

typedef struct GServer GServer;
struct GServer {
    int max_clients;
    int current_clients;

    int id;

    char *name;
};

char *get_client_to_server_fifo_name();

void gserver_init(GServer *gserver);

GServer *gserver_new();

#endif