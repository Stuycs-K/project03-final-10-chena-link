#include <poll.h>

#include "../network/clientlist.h"
#include "../shared.h"
#include "clientconnection.h"

#ifndef MAINSERVER_H
#define MAINSERVER_H

/*
    A macro to loop through all connected clients. Use END_FOREACHCLIENT() to close the loop.
    The Client variable is named "client."

    server : the Server object. You can't do FOREACH_CLIENT(gserver->server), though.
    You must first do Server *server = gserver->server and then FOREACH_CLIENT(server).
*/
#define FOREACH_CLIENT(server)                                                \
    for (int client_id = 0; client_id < (server)->max_clients; ++client_id) { \
        Client *client = (server)->clients[client_id];                        \
        if (client->is_free) {                                                \
            continue;                                                         \
        }

#define END_FOREACH_CLIENT() }

/*
    A base Server, which communicactes with clients. CServer and GServer both use this.
*/
typedef struct Server Server;
struct Server {
    pid_t pid; // The PID on which this server is running.

    int max_clients;     // The maximum number of clients that can connect to this server.
    int current_clients; // How many clients are currently connected.

    int id; // An identifier given by the central server

    char name[MAX_GSERVER_NAME_CHARACTERS]; // A name a client can give the server.
    char wkp_name[GSERVER_WKP_NAME_LEN];    // Given by the central server. Used by clients to connect.

    Client **clients; // FDs and queues for each client

    int client_info_changed;          // A flag to send client_info_list if a client joined / disconnected
    ClientInfoNode *client_info_list; // Networked to clients

    pid_t connection_handler_pid;                 // The PID of the connection handler. Used solely to do magic (see handle_client_connection).
    NetEventQueue *connection_handler_recv_queue; // A queue for the connection handler so that it can inform the server about clients connecting.
    int connection_handler_pipe[2];               // FDs to and from the connection handler.

    NetEventQueue *send_to_all_events; // A queue for holding NetEvents that have been sent to all clients. Needed to solve a stupid problem. See server_send_to_all
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

void server_shutdown(Server *this);

#endif
