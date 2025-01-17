#include <stdio.h>
#include <unistd.h>

#include "../network/pipehandshake.h"
#include "../util/file.h"
#include "baseclient.h"

/*
    Creates a BaseClient with the given username.

    PARAMS:
        char *name : the username

    RETURNS: the new BaseClient
*/
BaseClient *client_new(char *name) {
    BaseClient *this = malloc(sizeof(BaseClient));

    this->client_id = -1;
    this->from_server_fd = -1;
    this->to_server_fd = -1;

    strcpy(this->name, name);

    this->client_info_list = NULL;

    this->recv_queue = net_event_queue_new();
    this->send_queue = net_event_queue_new();

    return this;
}

/*
    Checks if the client can actively receive / send NetEvents to a server.

    PARAMS:
        BaseClient *this : the BaseClient

    RETURNS: 1 if both upstream and downstream FDs are valid, 0 otherwise.
*/
int client_is_connected(BaseClient *this) {
    return (this->from_server_fd != -1 && this->to_server_fd != -1);
}

/*
    Attempts to establish a connection with a server.

    PARAMS:
        BaseClient * this : the BaseClient
        char *wkp : the name of the WKP to perform a handshake through

    RETURNS: 1 if connection succeeded, -1 if the handshake failed.
*/
int client_connect(BaseClient *this, char *wkp) {
    NetEvent *handshake_event = create_handshake_event();
    Handshake *handshake = handshake_event->args;
    strcpy(handshake->client_name, this->name);

    client_setup(wkp, handshake_event);

    int succeeded = client_handshake(handshake_event);
    if (succeeded == -1) {
        printf("Connection to server failed\n");
        free_handshake_event(handshake_event);

        return -1;
    }

    this->to_server_fd = handshake->client_to_server_fd;
    this->from_server_fd = handshake->server_to_client_fd;
    set_nonblock(this->from_server_fd); // MUST SET NONBLOCK HERE

    free_handshake_event(handshake_event);
    return 1;
}

/*
    Updates the client's client_id and information about other clients connected.

    PARAMS:
        BaseClient * this : the BaseClient
        ClientList *nargs : the net args to update with

    RETURNS: none
*/
void on_recv_client_list(BaseClient *this, ClientList *nargs) {
    this->client_id = nargs->local_client_id;
    printf("Our client ID: %d\n", this->client_id);

    free_client_list(this->client_info_list);
    this->client_info_list = copy_client_list(nargs->info_list);

    print_client_list(this->client_info_list);
}

/*
    Deserializes and creates NetEvents for all incoming server events.
    Handles CLIENT_LIST.

    PARAMS:
        BaseClient * this : the BaseClient

    RETURNS: none
*/
void client_recv_from_server(BaseClient *this) {
    clear_event_queue(this->recv_queue);

    void *recv_buffer;
    while (recv_buffer = read_into_buffer(this->from_server_fd)) {
        recv_event_queue(this->recv_queue, recv_buffer);

        for (int i = 0; i < this->recv_queue->event_count; ++i) {
            NetEvent *event = this->recv_queue->events[i];
            void *args = event->args;

            switch (event->protocol) {

            case CLIENT_LIST: {
                on_recv_client_list(this, args);
                break;
            }

            default:
                break;
            }
        }
    }
}

/*
    Queues a NetEvent to be sent to the server.

    PARAMS:
        BaseClient * this : the BaseClient
        NetEvent *event : the NetEvent to queue up to the server

    RETURNS: none
*/
void client_send_event(BaseClient *this, NetEvent *event) {
    insert_event(this->send_queue, event);
}

/*
    Sends the event queue to the server.

    PARAMS:
        BaseClient * this : the BaseClient

    RETURNS: none
*/
void client_send_to_server(BaseClient *this) {
    send_event_queue(this->send_queue, this->to_server_fd);
    clear_event_queue(this->send_queue);
}

/*
    Disconnects from the server this is currently connected to.
    Clears event queues and invalidates FDs.

    PARAMS:
        BaseClient * this : the BaseClient

    RETURNS: none
*/
void client_disconnect_from_server(BaseClient *this) {
    if (!client_is_connected(this)) {
        return;
    }

    free_client_list(this->client_info_list); // Forget client list
    this->client_info_list = NULL;

    close(this->to_server_fd);
    close(this->from_server_fd);

    this->to_server_fd = -1;
    this->from_server_fd = -1;

    this->client_id = -1;

    clear_event_queue(this->recv_queue);
    clear_event_queue(this->send_queue);
}

/*
    Frees the client.

    PARAMS:
        BaseClient * this : the BaseClient

    RETURNS: none
*/
void free_client(BaseClient *this) {
    free_client_list(this->client_info_list);
    this->client_info_list = NULL;

    close(this->to_server_fd);
    close(this->from_server_fd);

    clear_event_queue(this->recv_queue);
    clear_event_queue(this->send_queue);

    free(this->recv_queue);
    free(this->send_queue);

    free(this);
}