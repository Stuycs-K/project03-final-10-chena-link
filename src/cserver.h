#include <sys/types.h>

#include "gserver.h"
#include "network/pipenet.h"

#ifndef CSERVER_H
#define CSERVER_H

typedef struct CServer CServer;
struct CServer {
    NetEvent *server_list_event;
    int server_list_updated;

    GServer **gserver_list;
    int gserver_count;
    Server *server;
};

CServer *cserver_new(int id);
void cserver_run(CServer *this);

// int cserver_init();

#endif
