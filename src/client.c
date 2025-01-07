#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "client.h"
#include "network/pipehandshake.h"
#include "network/piperw.h"

typedef struct PeriodicHandshakeArgs PeriodicHandshakeArgs;
struct PeriodicHandshakeArgs {
    int n;
};

void write_test(NetBuffer *nb, void *args) {
    PeriodicHandshakeArgs *pargs = (PeriodicHandshakeArgs *)args;

    NET_BUFFER_WRITE_VALUE(nb, pargs->n)

    printf("OFFSET %d\n", nb->offset);
}

void client_main(void) {
    int net_fds[2];
    // client_handshake("TEMP", net_fds);

    net_init();
    NetEventQueue *net_event_queue = net_event_queue_new();

    bind_send_event(PERIODIC_HANDSHAKE, write_test);

    NetEvent test_event;
    test_event.protocol = PERIODIC_HANDSHAKE;

    PeriodicHandshakeArgs test_args;
    test_args.n = 120;
    test_event.args = &test_args;

    insert_event(net_event_queue, &test_event);
    insert_event(net_event_queue, &test_event);

    send_event_queue(net_event_queue, STDOUT_FILENO);

    while (1) {
        printf("hello\n");
        usleep(1000000);
    }
}
