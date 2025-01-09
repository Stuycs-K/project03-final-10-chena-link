#include <sys/types.h>

#include "network/pipenet.h"

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

    int subserver_pipe[2];

    NetEvent *handshake_event; // Handshake to complete
};

typedef enum GServerStatus GServerStatus;
enum GServerStatus {
    GSS_WAITING_FOR_PLAYERS, // Not reached max_clients
    GSS_FULL,                // Server has reached max_clients
    GSS_STARTING,            // Host has started countdown
    GSS_GAME_IN_PROGRESS     // We're playing the game
};

typedef struct GServer GServer;
struct GServer {
    GServerStatus status;

    int max_clients;
    int current_clients;

    int id; // Given by the central server

    int subserver_pipe[2];

    char *name;

    GSubserver **subservers; // List of subserver connections
};

char *get_client_to_server_fifo_name();

void gserver_init(GServer *gserver);

GServer *gserver_new();

#endif