#include "../network/pipenet.h"
#include "subserver.h"

#ifndef CLIENTCONNECTION_H
#define CLIENTCONNECTION_H

typedef struct ClientConnection ClientConnection;
struct ClientConnection {
    int id;
    char *name;
    int send_fd;

    Subserver *subserver;

    NetEventQueue *send_queue;
    NetEventQueue *recv_queue;
};

#endif