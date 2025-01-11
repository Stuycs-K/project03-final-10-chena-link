#include <sys/types.h>

#include "network/pipenet.h"

#ifndef CSERVER_H
#define CSERVER_H

typedef struct CSubserver CSubserver;
struct CSubserver {
    pid_t pid; // Process ID this connection is running on

    int send_fd;
    int recv_fd;

    NetEvent *handshake_event; // Handshake to complete
};

// int cserver_init();

#endif
