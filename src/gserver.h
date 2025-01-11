#include <sys/types.h>

#include "network/pipenet.h"
#include "server/mainserver.h"

/*
    GServer, or game server handles players for a single game instance.
*/

#ifndef GSERVER_H
#define GSERVER_H

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

    Server *server;
};

void gserver_run(GServer *gserver);
GServer *gserver_new(int id);

#endif