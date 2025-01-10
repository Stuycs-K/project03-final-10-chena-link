#include "connectionhandler.h"
#include "../network/pipehandshake.h"

/*
void accept_connection(Server *this) {
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

    Subserver *subserver = setup_subserver_for_connection(this, handshake_event);

    pid_t pid = fork();

    if (pid == 0) {
        subserver_run(subserver);
    } else {
        this->subservers[subserver->client_id]->pid = pid;
        free_handshake_event(handshake_event);
    }
}
*/
