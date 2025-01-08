#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "client.h"
#include "network/pipehandshake.h"
#include "network/pipenet.h"
#include "network/pipenetevents.h"

void client_main(void) {
    int net_fds[2];
    client_handshake("TEMP", net_fds);

    int to_server = net_fds[0];
    int from_server = net_fds[1];

    net_init();
    NetEventQueue *net_send_queue = net_event_queue_new();

    while (1) {
        empty_net_event_queue(net_send_queue);

        for (int i = 0; i < 2; ++i) {
            NetArgs_PeriodicHandshake *test_args = malloc(sizeof(NetArgs_PeriodicHandshake));
            test_args->id = rand();

            printf("rand: %d\n", test_args->id);
            NetEvent *test_event = net_event_new(PERIODIC_HANDSHAKE, test_args);

            insert_event(net_send_queue, test_event);
        }

        send_event_queue(net_send_queue, to_server);

        printf("sent data\n");

        usleep(1000000);
    }
}
