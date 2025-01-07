#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "gserver.h"
#include "pipehandshake.h"

char *get_client_to_server_fifo_name() {
    // return getpid();
    return "TEMP";
}

void gserver_init() {
    int from_client;
    int to_client;

    while (1) {
        from_client = server_setup(get_client_to_server_fifo_name());
        to_client = server_handshake(from_client);

        printf("=================================\n");
        // send_loop(to_client, "PERSISTENT SERVER");
        printf("=================================\n");

        close(to_client);
        close(from_client);
    }
}