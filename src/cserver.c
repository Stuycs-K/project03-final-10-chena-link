#include <stdio.h>
#include <unistd.h>

#include "cserver.h"
#include "network/gserver.h"
#include "network/pipehandshake.h"
#include "network/pipenet.h"

GServer **game_server_list;

void csubserver_init(int recv_fd, int send_fd, NetEvent *handshake_event) {
    int status = server_complete_handshake(recv_fd, send_fd, handshake_event);

    free_handshake_event(handshake_event);
    handshake_event = NULL;

    printf("CLIENT CONNECTED\n");
}

int cserver_init() {
    game_server_list = malloc(sizeof(GServer *) * 256);

    int from_client;

    while (1) {
        from_client = server_setup("CSERVER");

        NetEvent *handshake_event = create_handshake_event();
        int to_client = server_get_send_fd(from_client, handshake_event);

        pid_t pid = fork();

        if (pid == 0) {
            csubserver_init(from_client, to_client, handshake_event);
        }
    }


    return 0;
}
