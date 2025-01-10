#include <stdio.h>
#include <unistd.h>

#include "../network/pipehandshake.h"
#include "connectionhandler.h"

void connection_handler_init(Server *this) {
    NetEvent *handshake_event = server_setup("TEMP");
    if (handshake_event == NULL) { // No clients attempting to connect on this tick.
        return;
    }

    server_get_send_fd(handshake_event);

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
    } else {
        free_handshake_event(handshake_event);
    }
}
