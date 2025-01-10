#include <stdio.h>
#include <unistd.h>

#include "../network/pipehandshake.h"
#include "connectionhandler.h"
#include "mainserver.h"
#include "subserver.h"

void server_run_connection_loop(Server *server) {
    while (1) {
        int from_client = server_setup("TEMP");

        NetEvent *handshake_event = create_handshake_event();
        ((NetArgs_InitialHandshake *)handshake_event->args)->client_to_server_fd = from_client;

        server_get_send_fd(handshake_event);

        // A client connected, but the server is full!
        if (server->current_clients >= server->max_clients) {
            server_abort_handshake(handshake_event, HEC_SERVER_IS_FULL);
            free_handshake_event(handshake_event);

            printf("server is full!\n");
            continue;
        }

        Subserver *subserver = server_setup_subserver_for_connection(server, handshake_event);
        if (subserver == NULL) {
            continue;
        }

        pid_t pid = fork();

        if (pid == 0) {
            subserver_run(subserver);
        } else {
            server->subservers[subserver->client_id]->pid = pid;
            free_handshake_event(handshake_event);
        }
    }
}