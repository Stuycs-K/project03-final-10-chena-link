#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "client.h"
#include "network/pipehandshake.h"
#include "network/pipenet.h"
#include "network/pipenetevents.h"

void client_main(void) {
    int net_fds[2];
    // client_handshake("TEMP", net_fds);

    net_init();
    NetEventQueue *net_event_queue = net_event_queue_new();

    NetArgs_PeriodicHandshake test_args;
    test_args.id = 120;

    NetEvent *test_event = net_event_new(PERIODIC_HANDSHAKE, &test_args);

    insert_event(net_event_queue, test_event);
    insert_event(net_event_queue, test_event);
    insert_event(net_event_queue, test_event);

    send_event_queue(net_event_queue, STDOUT_FILENO);

    while (1) {
        printf("hello\n");
        usleep(1000000);
    }
}
