#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "client.h"
#include "network/pipehandshake.h"
#include "network/pipenet.h"
#include "network/pipenetevents.h"

void client_main(void) {
    net_init();

    NetEvent *handshake_event = create_handshake_event();

    int to_server, from_server;

    to_server = client_setup("TEMP", handshake_event);
    from_server = client_handshake(to_server, handshake_event);

    if (from_server == -1) {
        printf("Connection failed\n");
        return;
    }

    NetEventQueue *net_send_queue = net_event_queue_new();

    while (1) {
        usleep(1000000);
    }
}
