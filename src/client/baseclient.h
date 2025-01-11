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

    NetEventQueue *recv_queue;
    NetEventQueue *send_queue;
};

BaseClient *base_client_new();

#endif