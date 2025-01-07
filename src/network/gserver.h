#include <sys/types.h>

/*
    GServer, or game server handles players for a single game instance.
*/

#ifndef GSERVER_H
#define GSERVER_H

// GSubserver handles communication with ONE client.
typedef struct GSubserver GSubserver;
struct GSubserver {
    pid_t pid; // Process ID this connection is running on

    int client_id;
    int send_fd;
    int recv_fd;
};

typedef struct GServer GServer;
struct GServer {
    int max_clients;
    int current_clients;

    int id; // Given by the central server

    char *name;

    GSubserver **subservers; // List of subserver connections
};

char *get_client_to_server_fifo_name();

void gserver_init(GServer *gserver);

GServer *gserver_new();

#endif