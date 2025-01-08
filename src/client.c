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

    int client_id = ((NetArgs_InitialHandshake *)handshake_event->args)->client_id;
    printf("This is %d\n", client_id);

    if (from_server == -1) {
        printf("Connection failed\n");
        return;
    }

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
