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
    NetEvent *info_event;
    NetEventQueue *cserver_send_queue;
    NetEventQueue *cserver_recv_queue;

    GServerStatus status;
    int cserver_pipes[2];

    Server *server;
};

void gserver_run(GServer *gserver);
GServer *gserver_new(int id);

#endif