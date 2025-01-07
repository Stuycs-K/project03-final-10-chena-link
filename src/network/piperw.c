#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "piperw.h"

NetEventHandler *g_net_event_handlers[PROTOCOL_COUNT];

NetEventQueue *net_event_queue_new() {
    NetEventQueue *net_event_queue = malloc(sizeof(NetEventQueue));

    net_event_queue->event_count = 0;
    net_event_queue->max_events = 255;

    net_event_queue->events = malloc(sizeof(NetEvent *) * net_event_queue->max_events);

    return net_event_queue;
}

void insert_event(NetEventQueue *net_event_queue, NetEvent *event) {
    if (net_event_queue->event_count >= net_event_queue->max_events) {
        printf("Queue full! Dropping event\n");
        return;
    }

    net_event_queue->events[net_event_queue->event_count++] = event;
}

/*

*/
void send_event_queue(NetEventQueue *net_event_queue, int send_fd) {
    BEGIN_SEND_BUFFER()
    SEND(net_event_queue->event_count)

    for (int i = 0; i < net_event_queue->event_count; ++i) {
        NetEvent *event_to_send = net_event_queue->events[i];

        NetProtocol protocol = event_to_send->protocol;
        void *args = event_to_send->args;

        NetEventHandler *handler = g_net_event_handlers[protocol];
        printf("%d\n", handler->protocol);
        if (handler->write_fn == NULL) {
            printf("uh oh\n");
        }
        handler->write_fn(args, send_buffer, offset, current_send_buf_size);
    }

    TRANSMIT_SEND_BUFFER(send_fd)
    END_SEND_BUFFER()
}

void recv_event_queue(NetEventQueue *net_event_queue) {}

void bind_send_event(NetProtocol protocol, NetEventWriter writer) {
    NetEventHandler *existing_handler = g_net_event_handlers[protocol];
    if (existing_handler != NULL) {
        if (existing_handler->write_fn != NULL) {
            printf("NetEventHandler already has a bound writer\n");
            return;
        }

        existing_handler->write_fn = writer;
        return;
    }

    g_net_event_handlers[protocol] = malloc(sizeof(NetEventHandler));
    g_net_event_handlers[protocol]->protocol = protocol;
    g_net_event_handlers[protocol]->write_fn = writer;
}

void bind_recv_event(NetProtocol protocol, NetEventReader reader) {
    NetEventHandler *existing_handler = g_net_event_handlers[protocol];
    if (existing_handler != NULL) {
        printf("NetEventHandler already has a bound reader\n");
        return;
    }

    existing_handler->read_fn = reader;
}

void net_init() {
    for (int i = 0; i < PROTOCOL_COUNT; ++i) {
        g_net_event_handlers[i] = NULL;
    }
}