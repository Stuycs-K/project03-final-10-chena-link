#include <stdio.h>
#include <unistd.h>

#include "cserver.h"
#include "gserver.h"
#include "network/pipehandshake.h"
#include "network/pipenet.h"

GServer **game_server_list;

void csubserver_init(NetEvent *handshake_event) {
    int status = server_complete_handshake(handshake_event);

    free_handshake_event(handshake_event);
    handshake_event = NULL;

    printf("CLIENT CONNECTED\n");
}

int cserver_init() {
    game_server_list = malloc(sizeof(GServer *) * 256);

    while (1) {
        int from_client = server_setup("CSERVER");

        NetEvent *handshake_event = create_handshake_event();
        ((NetArgs_InitialHandshake *)handshake_event->args)->client_to_server_fd = from_client;

        server_get_send_fd(handshake_event);

        pid_t pid = fork();

        if (pid == 0) {
            csubserver_init(handshake_event);
        }
    }

    return 0;
}
