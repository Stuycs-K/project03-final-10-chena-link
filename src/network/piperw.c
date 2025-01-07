#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "piperw.h"

NetEventHandler *g_net_event_handlers[PROTOCOL_COUNT];

NetBuffer *net_buffer_new() {
    NetBuffer *new_net_buffer = malloc(sizeof(NetBuffer));

    new_net_buffer->size = 512;
    new_net_buffer->buffer = malloc(sizeof(char) * 512);
    new_net_buffer->offset = 0;

    return new_net_buffer;
}

void transmit_net_buffer(NetBuffer *net_buffer, int target_fd) {
    write(target_fd, net_buffer->buffer, net_buffer->offset);
}

void free_net_buffer(NetBuffer *net_buffer) {
    free(net_buffer->buffer);
    free(net_buffer);
}

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

void send_event_queue(NetEventQueue *net_event_queue, int send_fd) {
    NetBuffer *nb = net_buffer_new();
    NET_BUFFER_WRITE_VALUE(nb, net_event_queue->event_count) // Write event count

    for (int i = 0; i < net_event_queue->event_count; ++i) {
        NetEvent *event_to_send = net_event_queue->events[i];

        NetProtocol protocol = event_to_send->protocol;
        void *args = event_to_send->args;

        NetEventHandler *handler = g_net_event_handlers[protocol];

        NET_BUFFER_WRITE_VALUE(nb, protocol) // Write the event's protocol

        if (handler->write_fn == NULL) {
            printf("Protocol handler has no write function attached!\n");
            return;
        }
        handler->write_fn(nb, args);
    }

    transmit_net_buffer(nb, send_fd);
    free_net_buffer(nb);
}

void recv_event_queue(NetEventQueue *net_event_queue, void *recv_buffer) {
    NetBuffer *nb = net_buffer_new();

    int event_count;
}

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
        if (existing_handler->read_fn != NULL) {
            printf("NetEventHandler already has a bound reader\n");
            return;
        }

        existing_handler->read_fn = reader;
        return;
    }

    g_net_event_handlers[protocol] = malloc(sizeof(NetEventHandler));
    g_net_event_handlers[protocol]->protocol = protocol;
    g_net_event_handlers[protocol]->read_fn = reader;
}

void net_init() {
    for (int i = 0; i < PROTOCOL_COUNT; ++i) {
        g_net_event_handlers[i] = NULL;
    }
}
