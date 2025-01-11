#include <stdio.h>
#include <unistd.h>

#include "../network/pipehandshake.h"
#include "../network/pipenet.h"
#include "../shared.h"
#include "connectionhandler.h"

/*
    Listens to WKP. When a client opens it, they complete the handshake.

    Then, check the server to see if we should accept the client.

    If they send the correct ACK, send the handshake struct back to the host server
    to inform them that the client connected.
*/
void connection_handler_init(Server *this) {
    NetEventQueue *send_host_queue = net_event_queue_new();

    int recv_from_host_server_fd = this->connection_handler_pipe[PIPE_READ];
    int send_to_host_server_fd = this->connection_handler_pipe[PIPE_WRITE];

    while (1) {
        NetEvent *handshake_event = server_setup("TEMP");
        NetArgs_Handshake *handshake = handshake_event->args;

        server_get_send_fd(handshake_event);

        // TODO: Used shared memory to check this?
        // A client connected, but the server is full!
        if (this->current_clients >= this->max_clients) {
            server_abort_handshake(handshake_event, HEC_SERVER_IS_FULL);
            free_handshake_event(handshake_event);

            printf("server is full!\n");
            return;
        }

        pid_t pid = fork();

        if (pid == 0) {
            int status = server_complete_handshake(handshake_event);

            if (status == -1) {
                printf("ACK Fail\n");
                exit(EXIT_FAILURE);
            }

            handshake->are_fds_finalized = 1;
            send_event_immediate(handshake_event, send_to_host_server_fd);

            exit(EXIT_SUCCESS);
        } else {
            free_handshake_event(handshake_event);
        }
    }
}
