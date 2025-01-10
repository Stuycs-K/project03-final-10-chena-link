#include "../network/pipenet.h"

#ifndef SUBSERVER_H
#define SUBSERVER_H

typedef struct Subserver Subserver;
struct Subserver {
    pid_t pid; // Process ID this connection is running on

    int client_id;
    int send_fd;
    int recv_fd;

    int main_pipe[2];

    NetEvent *handshake_event; // Handshake to complete
};

Subserver *subserver_new(int client_id);
int subserver_is_inactive(Subserver *this);
void subserver_run(Subserver *this);

#endif