#include <poll.h>

#include "clientconnection.h"

#ifndef MAINSERVER_H
#define MAINSERVER_H

typedef struct Server Server;
struct Server {
    int max_clients;
    int current_clients;

    int id; // Given by the central server

    char *name;

    ClientConnection **clients;
    int *recv_fd_list;
    int connection_handler_pipe[2];
};

typedef struct PollRequestList PollRequestList;
struct PollRequestList {
    int count;
    struct pollfd *poll_requests;
};

Server *server_new(int server_id);
void server_set_max_clients(Server *this, int max_clients);
void server_run(Server *this);

#endif
