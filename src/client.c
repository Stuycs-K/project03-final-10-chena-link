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
    NetArgs_InitialHandshake *handshake = handshake_event->args;

    client_setup("TEMP", handshake_event);
    int succeeded = client_handshake(handshake_event);

    if (succeeded == -1) {
        printf("Connection failed\n");
        return;
    }

    int client_id = handshake->client_id;
    int to_server = handshake->client_to_server_fd;
    int from_server = handshake->server_to_client_fd;

    printf("This is %d\n", client_id);

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
