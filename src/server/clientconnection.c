#include <stdio.h>

#include "clientconnection.h"
#include "../network/pipehandshake.h"
#include "../network/pipenetevents.h"
#include "../shared.h"

ClientConnection *client_connection_new(int id) {
    ClientConnection *this = malloc(sizeof(ClientConnection));

    this->id = id;
    this->name = calloc(sizeof(char), MAX_PLAYER_NAME_CHARACTERS);
    this->send_fd = -1;
    this->recv_fd = -1;

    this->is_free = CONNECTION_IS_FREE;

    this->send_queue = net_event_queue_new();
    this->recv_queue = net_event_queue_new();

    return this;
}

int complete_handshake(ClientConnection *this, NetEvent *handshake_event) {
    NetArgs_InitialHandshake *handshake = handshake_event->args;

    this->recv_fd = handshake->client_to_server_fd;
    this->send_fd = handshake->server_to_client_fd;

    int status = server_complete_handshake(handshake_event);

    free_handshake_event(handshake_event);

    if (status == -1) {
        printf("ACK Fail\n");
        return -1;
    }

    return 1;
}

void disconnect_client(ClientConnection *this) {
    this->is_free = CONNECTION_IS_FREE;
    memset(this->name, 0, MAX_PLAYER_NAME_CHARACTERS);

    // Close them first?
    this->send_fd = -1;
    this->recv_fd = -1;

    empty_net_event_queue(this->send_queue);
    empty_net_event_queue(this->recv_queue);
}

void free_client_connection(ClientConnection *this) {
    free(this->name);
    free(this->send_queue);
    free(this->recv_queue);
    free(this);
}
