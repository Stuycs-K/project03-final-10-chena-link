#include <sys/types.h>

/*
    GServer, or game server handles players for a single game instance.
*/

#ifndef GSERVER_H
#define GSERVER_H

// GSubserver handles communication with ONE client.
typedef struct GSubserver GSubserver;
struct GSubserver {
    pid_t pid;

    // If any 3 of the following are -1, the subserver is considered inactive
    int client_id;
    int send_fd;
    int recv_fd;
};

typedef struct GServer GServer;
struct GServer {
    int max_clients;
    int current_clients;

    int id;

    char *name;

    GSubserver **subservers; // List of subserver connections
    struct card p1[100];
    struct card p2[100];
};

typedef struct card card;
struct card{
  int color;
  int num;
}

char *get_client_to_server_fifo_name();

void gserver_init(GServer *gserver);

GServer *gserver_new();

#endif
