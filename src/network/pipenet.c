#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "pipenet.h"
#include "pipenetevents.h"

#define BIND(protocol, internal_name)             \
    handlers[protocol] = handler_##internal_name; \
    destructors[protocol] = destroy_##internal_name;

NetEventHandler handlers[PROTOCOL_COUNT];
NetEventDestructor destructors[PROTOCOL_COUNT];

NetBuffer *net_buffer_send() {
    NetBuffer *new_net_buffer = malloc(sizeof(NetBuffer));

    new_net_buffer->size = 512;
    new_net_buffer->buffer = malloc(sizeof(char) * 512);
    new_net_buffer->offset = 0;

    return new_net_buffer;
}

NetBuffer *net_buffer_recv(void *buffer) {
    NetBuffer *new_net_buffer = malloc(sizeof(NetBuffer));

    new_net_buffer->size = 0;
    new_net_buffer->buffer = buffer;
    new_net_buffer->offset = 0;

    return new_net_buffer;
}

void transmit_net_buffer(NetBuffer *net_buffer, int target_fd) {
    ssize_t bytes_written = write(target_fd, net_buffer->buffer, net_buffer->offset);
    if (bytes_written <= 0) {
        printf("%d\n", target_fd);
        perror("oops");
    }
}

void free_net_buffer(NetBuffer *net_buffer) {
    free(net_buffer->buffer);
    free(net_buffer);
}

NetEvent *net_event_new(NetProtocol protocol, void *args) {
    NetEvent *net_event = malloc(sizeof(NetEvent));
    net_event->protocol = protocol;
    net_event->args = args;
    net_event->cleanup_behavior = NEVENT_BASE;
    return net_event;
}

NetEventQueue *net_event_queue_new() {
    NetEventQueue *net_event_queue = malloc(sizeof(NetEventQueue));

    net_event_queue->event_count = 0;
    net_event_queue->max_events = 255;

    net_event_queue->events = malloc(sizeof(NetEvent *) * net_event_queue->max_events);
    net_event_queue->attached_events = calloc(sizeof(NetEvent *), PROTOCOL_COUNT);

    return net_event_queue;
}

void insert_event(NetEventQueue *net_event_queue, NetEvent *event) {
    if (net_event_queue->event_count >= net_event_queue->max_events) {
        return;
    }

    net_event_queue->events[net_event_queue->event_count++] = event;
}

void attach_event(NetEventQueue *net_event_queue, NetEvent *event) {
    event->cleanup_behavior = NEVENT_PERSISTENT;
    net_event_queue->attached_events[event->protocol] = event;
}

void detach_event(NetEventQueue *net_event_queue, NetEvent *event) {
    net_event_queue->attached_events[event->protocol] = NULL;
}

void empty_net_event_queue(NetEventQueue *net_event_queue) {
    for (int i = 0; i < net_event_queue->event_count; ++i) {
        NetEvent *event = net_event_queue->events[i];

        switch (event->cleanup_behavior) {

        case NEVENT_PERSISTENT: // Simply remove it from the queue
            net_event_queue->events[i] = NULL;
            break;

        case NEVENT_PERSISTENT_ARGS:
            event->args = NULL;               // Don't free the args!
            free(net_event_queue->events[i]); // Do free the event wrapper
            net_event_queue->events[i] = NULL;
            break;

        case NEVENT_BASE:
            destructors[event->protocol](event->args); // Call destructor
            free(net_event_queue->events[i]);
            net_event_queue->events[i] = NULL;

        default:
            break;
        }
    }

    net_event_queue->event_count = 0;
}

void send_event_queue(NetEventQueue *net_event_queue, int send_fd) {
    if (net_event_queue->event_count == 0) {
        return;
    }

    for (int i = 0; i < PROTOCOL_COUNT; ++i) {
        NetEvent *attached_event = net_event_queue->attached_events[i];
        if (attached_event != NULL) {
            insert_event(net_event_queue, attached_event);
        }
    }

    NetBuffer *nb = net_buffer_send();

    NET_BUFFER_BEGIN_WRITE(nb)

    NET_BUFFER_WRITE_VALUE(nb, net_event_queue->event_count) // Write event count

    for (int i = 0; i < net_event_queue->event_count; ++i) {
        NetEvent *event_to_send = net_event_queue->events[i];

        NetProtocol protocol = event_to_send->protocol;
        void *args = event_to_send->args;

        NET_BUFFER_WRITE_VALUE(nb, protocol) // Write the event's protocol
        handlers[protocol](nb, args, 0);
    }

    NET_BUFFER_END_WRITE(nb)

    transmit_net_buffer(nb, send_fd);
    free_net_buffer(nb);
}

void send_event_immediate(NetEvent *event, int send_fd) {
    NetBuffer *nb = net_buffer_send();

    NetProtocol protocol = event->protocol;
    void *args = event->args;

    int throwaway_size = 1;

    NET_BUFFER_BEGIN_WRITE(nb)

    NET_BUFFER_WRITE_VALUE(nb, throwaway_size) // To maintain compatability with recv_event_queue
    NET_BUFFER_WRITE_VALUE(nb, protocol)
    handlers[protocol](nb, args, 0);

    NET_BUFFER_END_WRITE(nb)

    transmit_net_buffer(nb, send_fd);
    free_net_buffer(nb);
}

void *read_into_buffer(int recv_fd) {
    ssize_t bytes_read;

    size_t packet_size = 0;
    bytes_read = read(recv_fd, &packet_size, sizeof(packet_size));

    if (bytes_read <= 0) {
        return NULL;
    }

    char *recv_buffer = malloc(sizeof(char) * packet_size);
    read(recv_fd, recv_buffer, packet_size);

    if (bytes_read <= 0) {
        perror("read_into_buffer: read rest of packet");
    }

    return recv_buffer;
}

// Populates a NetEventQueue with deserialized NetEvents
void recv_event_queue(NetEventQueue *net_event_queue, void *recv_buffer) {
    NetBuffer *nb = net_buffer_recv(recv_buffer);

    int event_count;
    NET_BUFFER_READ_VALUE(nb, event_count)

    for (int i = 0; i < event_count; ++i) {
        NetProtocol protocol;
        NET_BUFFER_READ_VALUE(nb, protocol);

        // Serialize into the attached event's args directly.
        NetEvent *attached_event = net_event_queue->attached_events[protocol];
        if (attached_event != NULL) {
            handlers[protocol](nb, attached_event->args, 1);
            continue;
        }

        void *data = handlers[protocol](nb, NULL, 1);
        insert_event(net_event_queue, net_event_new(protocol, data));
    }

    free_net_buffer(nb);
}

NetEvent *recv_event_immediate(int recv_fd, NetEvent *recv_event) {
    void *recv_buffer = read_into_buffer(recv_fd);
    NetBuffer *nb = net_buffer_recv(recv_buffer);

    int throwaway_size;
    NET_BUFFER_READ_VALUE(nb, throwaway_size);

    NetProtocol protocol;
    NET_BUFFER_READ_VALUE(nb, protocol);

    void *data;
    if (recv_event == NULL) { // Create our own NetEvent
        data = handlers[protocol](nb, NULL, 1);
        free_net_buffer(nb);

        return net_event_new(protocol, data);
    } else {
        data = handlers[protocol](nb, recv_event->args, 1);
        free_net_buffer(nb);

        return recv_event;
    }
}

void net_init() {
    BIND(PERIODIC_HANDSHAKE, periodic_handshake);
    BIND(HANDSHAKE, handshake);
    BIND(CLIENT_LIST, client_list);
    BIND(GSERVER_INFO, gserver_info);
    BIND(GSERVER_LIST, gserver_info_list);
}
