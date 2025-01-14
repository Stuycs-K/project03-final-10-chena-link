#include <sys/types.h>

#include "network/pipenet.h"
#include "server/baseserver.h"

#ifndef GSERVER_H
#define GSERVER_H

/*
    GServer (the G stands for game) handles players for a single game instance.
*/
typedef struct GServer GServer;
struct GServer {
    NetEvent *info_event;              // Wraps GServerInfo, used to tell the CServer about any updates
    NetEventQueue *cserver_send_queue; // Communication to the CServer
    NetEventQueue *cserver_recv_queue; // Communication from the CServer

    int host_client_id; // Who the host is (who we send GServerConfig to start the game)

    GServerStatus status; // See GServerStatus
    int cserver_pipes[2]; // Pipes to and from the CServer
    int decks[2];
    int SHMID;

    Server *server; // Internal Server object
};

void gserver_run(GServer *this);
GServer *gserver_new(int id);

#endif
