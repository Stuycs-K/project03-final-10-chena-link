#include <poll.h>

#include "../util/poll_list.h"
#include "clientconnection.h"

#ifndef MAINSERVER_H
#define MAINSERVER_H

typedef struct Server Server;
struct Server {
    int max_clients;
    int current_clients;

    int id; // Given by the central server

    char *name;

    Client **clients;
    PollList *client_poll_list;

    pid_t connection_handler_pid;
    NetEventQueue *connection_handler_recv_queue;

    int *recv_fd_list;
    int connection_handler_pipe[2];
};

Server *server_new(int server_id);
void server_set_max_clients(Server *this, int max_clients);
void server_run(Server *this);
void server_send_events(Server *this);

#endif
