#include <poll.h>

#include "../network/clientlist.h"
#include "clientconnection.h"

#ifndef MAINSERVER_H
#define MAINSERVER_H

#define FOREACH_CLIENT(server)                                                \
    for (int client_id = 0; client_id < (server)->max_clients; ++client_id) { \
        Client *client = (server)->clients[client_id];                        \
        if (client->is_free) {                                                \
            continue;                                                         \
        }

#define END_FOREACH_CLIENT }

typedef enum ServerStatus ServerStatus;
enum ServerStatus {
    SSTATUS_OPEN,
    SSTATUS_CLOSED,
};

typedef struct Server Server;
struct Server {
    ServerStatus status;

    int max_clients;
    int current_clients;

    int id; // Given by the central server

    char *name;

    Client **clients; // FDs and queues for each client

    int client_info_changed;          // A flag to send client_info_list if a client joined / disconnected
    ClientInfoNode *client_info_list; // Networked to clients

    pid_t connection_handler_pid;
    NetEventQueue *connection_handler_recv_queue;

    int *recv_fd_list;
    int connection_handler_pipe[2];
};

Server *server_new(int server_id);
void server_start_connection_handler(Server *this);
void server_set_max_clients(Server *this, int max_clients);
void server_run(Server *this);
void handle_core_server_net_event(Server *this, int client_id, NetEvent *event);
void handle_connections(Server *this);
void server_recv_events(Server *this);
void server_send_events(Server *this);
void server_empty_recv_events(Server *this);

void server_send_event_to(Server *this, int client_id, NetEvent *event);
void server_send_event_to_all(Server *this, NetEvent *event);

#endif
