#include <sys/types.h>

#include "network/pipenet.h"
#include "server/mainserver.h"

/*
    GServer, or game server handles players for a single game instance.
*/

#ifndef GSERVER_H
#define GSERVER_H

typedef struct GServer GServer;
struct GServer {
    GServerStatus status;
    int cserver_pipes[2];

    Server *server;
    int decks[2];
};

void gserver_run(GServer *gserver);
GServer *gserver_new(int id);

#endif