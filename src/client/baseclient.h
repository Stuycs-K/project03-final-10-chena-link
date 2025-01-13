/*
    Represents the client-side connection to a server.
*/

#include "../network/pipenet.h"
#include "../network/pipenetevents.h"

#ifndef BASECLIENT_H
#define BASECLIENT_H

/*
    BaseClient represents a client-side connection to a single server.
*/
typedef struct BaseClient BaseClient;
struct BaseClient {
    int client_id;                         // The client ID the server assigned us
    char name[MAX_PLAYER_NAME_CHARACTERS]; // Our username

    int to_server_fd;   // Self explanatory
    int from_server_fd; // Self explanatory

    ClientInfoNode *client_info_list; // List of information about other clients

    NetEventQueue *recv_queue; // Queue for receiving events from the server
    NetEventQueue *send_queue; // Queue for sending events to the server
};

BaseClient *client_new(char *name);

int client_is_connected(BaseClient *this);

int client_connect(BaseClient *this, char *wkp);

void on_recv_client_list(BaseClient *this, ClientList *nargs);

void client_recv_from_server(BaseClient *this);

void client_send_event(BaseClient *this, NetEvent *event);

void client_send_to_server(BaseClient *this);

void client_disconnect_from_server(BaseClient *this);

void free_client(BaseClient *this);

#endif