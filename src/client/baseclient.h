/*
    Represents the client-side connection to a server.
*/

#include "../network/pipenet.h"
#include "../network/pipenetevents.h"

#ifndef BASECLIENT_H
#define BASECLIENT_H

typedef struct BaseClient BaseClient;
struct BaseClient {
    int client_id;

    int to_server_fd;
    int from_server_fd;

    ClientInfoNode *client_info_list;

    NetEventQueue *recv_queue;
    NetEventQueue *send_queue;
};

BaseClient *client_new();

int client_connect(BaseClient *this, char *wkp);

void on_recv_client_list(BaseClient *this, ClientList *nargs);

void client_recv_from_server(BaseClient *this);

void client_send_event(BaseClient *this, NetEvent *event);

void client_send_to_server(BaseClient *this);

void free_client(BaseClient *this);

#endif