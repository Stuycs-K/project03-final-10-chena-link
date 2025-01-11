#include <stdio.h>
#include <unistd.h>

#include "../network/pipehandshake.h"
#include "../util/file.h"
#include "baseclient.h"

BaseClient *client_new() {
    BaseClient *this = malloc(sizeof(BaseClient));
    this->client_id = -1;
    this->from_server_fd = -1;
    this->to_server_fd = -1;

    this->client_info_list = NULL;

    this->recv_queue = net_event_queue_new();
    this->send_queue = net_event_queue_new();

    return this;
}

// Performs handshake
void client_connect(BaseClient *this, char *wkp) {
    NetEvent *handshake_event = create_handshake_event();
    NetArgs_Handshake *handshake = handshake_event->args;

    client_setup(wkp, handshake_event);

    int succeeded = client_handshake(handshake_event);
    if (succeeded == -1) {
        printf("Connection to server failed\n");
        free_handshake_event(handshake_event);

        return;
    }

    this->to_server_fd = handshake->client_to_server_fd;
    this->from_server_fd = handshake->server_to_client_fd;
    set_nonblock(this->from_server_fd); // MUST SET NONBLOCK HERE

    free_handshake_event(handshake_event);
}

void on_recv_client_list(BaseClient *this, ClientList *nargs) {
    this->client_id = nargs->local_client_id;
    printf("Our client ID: %d\n", this->client_id);

    free_client_list(this->client_info_list);
    this->client_info_list = copy_client_list(nargs->info_list);

    print_client_list(this->client_info_list);
}

void client_recv_from_server(BaseClient *this) {
    empty_net_event_queue(this->recv_queue);

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

void client_send_event(BaseClient *this, NetEvent *event) {
    insert_event(this->send_queue, event);
}

void client_send_to_server(BaseClient *this) {
    send_event_queue(this->send_queue, this->to_server_fd);
    empty_net_event_queue(this->send_queue);
}

void free_client(BaseClient *this) {
    free_client_list(this->client_info_list);

    close(this->to_server_fd);
    close(this->from_server_fd);

    empty_net_event_queue(this->recv_queue);
    empty_net_event_queue(this->send_queue);

    free(this->recv_queue);
    free(this->send_queue);

    free(this);
}