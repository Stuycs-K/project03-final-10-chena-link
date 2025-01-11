#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "pipenet.h"
#include "pipenetevents.h"

NetEventHandler *net_event_handlers[PROTOCOL_COUNT];

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
    net_event->is_persistent = 0;
    return net_event;
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

void empty_net_event_queue(NetEventQueue *net_event_queue) {
    for (int i = 0; i < net_event_queue->event_count; ++i) {
        NetEvent *event = net_event_queue->events[i];

        if (!event->is_persistent) {
            free(event->args);
            free(net_event_queue->events[i]);
        }

        net_event_queue->events[i] = NULL;
    }

    net_event_queue->event_count = 0;
}

void check_handler_write_fn_exists(NetEventHandler *handler) {
    if (handler->write_fn == NULL) {
        fprintf(
            stderr,
            "No write function bound to protocol (% d)! Make sure to bind with bind_send_event !\n",
            handler->protocol);
        exit(EXIT_FAILURE);
    }
}

void check_handler_read_fn_exists(NetEventHandler *handler) {
    if (handler->read_fn == NULL) {
        printf("yeah i like reading\n");
        fprintf(
            stderr,
            "No read function bound to protocol (% d)! Make sure to bind with bind_recv_event !\n",
            handler->protocol);
        exit(EXIT_FAILURE);
    }
}

void send_event_queue(NetEventQueue *net_event_queue, int send_fd) {
    if (net_event_queue->event_count == 0) {
        return;
    }

    NetBuffer *nb = net_buffer_send();

    NET_BUFFER_BEGIN_WRITE(nb)

    NET_BUFFER_WRITE_VALUE(nb, net_event_queue->event_count) // Write event count

    for (int i = 0; i < net_event_queue->event_count; ++i) {
        NetEvent *event_to_send = net_event_queue->events[i];

        NetProtocol protocol = event_to_send->protocol;
        void *args = event_to_send->args;

        NetEventHandler *handler = net_event_handlers[protocol];
        check_handler_write_fn_exists(handler);

        NET_BUFFER_WRITE_VALUE(nb, protocol) // Write the event's protocol
        handler->write_fn(nb, args);
    }

    NET_BUFFER_END_WRITE(nb)

    transmit_net_buffer(nb, send_fd);
    free_net_buffer(nb);
}

void send_event_immediate(NetEvent *event, int send_fd) {
    NetBuffer *nb = net_buffer_send();

    NetProtocol protocol = event->protocol;
    void *args = event->args;

    NetEventHandler *handler = net_event_handlers[protocol];
    check_handler_write_fn_exists(handler);

    int throwaway_size = 1;

    NET_BUFFER_BEGIN_WRITE(nb)

    NET_BUFFER_WRITE_VALUE(nb, throwaway_size) // To maintain compatability with recv_event_queue
    NET_BUFFER_WRITE_VALUE(nb, protocol)
    handler->write_fn(nb, args);

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

        NetEventHandler *handler = net_event_handlers[protocol];
        check_handler_read_fn_exists(handler);

        void *data = handler->read_fn(nb, NULL);
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

    NetEventHandler *handler = net_event_handlers[protocol];
    check_handler_read_fn_exists(handler);

    void *data;
    if (recv_event == NULL) { // Create our own NetEvent
        data = handler->read_fn(nb, NULL);
        free_net_buffer(nb);

        return net_event_new(protocol, data);
    } else {
        data = handler->read_fn(nb, recv_event->args);
        free_net_buffer(nb);

        return recv_event;
    }
}

void bind_send_event(NetProtocol protocol, NetEventWriter writer) {
    NetEventHandler *existing_handler = net_event_handlers[protocol];
    if (existing_handler != NULL) {
        if (existing_handler->write_fn != NULL) {
            printf("NetEventHandler already has a bound writer\n");
            return;
        }

        existing_handler->write_fn = writer;
        return;
    }

    net_event_handlers[protocol] = malloc(sizeof(NetEventHandler));
    net_event_handlers[protocol]->protocol = protocol;
    net_event_handlers[protocol]->write_fn = writer;
}

void bind_recv_event(NetProtocol protocol, NetEventReader reader) {
    NetEventHandler *existing_handler = net_event_handlers[protocol];
    if (existing_handler != NULL) {
        if (existing_handler->read_fn != NULL) {
            printf("NetEventHandler already has a bound reader\n");
            return;
        }

        existing_handler->read_fn = reader;
        return;
    }

    net_event_handlers[protocol] = malloc(sizeof(NetEventHandler));
    net_event_handlers[protocol]->protocol = protocol;
    net_event_handlers[protocol]->read_fn = reader;
}

void net_init() {
    for (int i = 0; i < PROTOCOL_COUNT; ++i) {
        net_event_handlers[i] = NULL;
    }

    bind_send_event(PERIODIC_HANDSHAKE, send_periodic_handshake);
    bind_recv_event(PERIODIC_HANDSHAKE, recv_periodic_handshake);

    bind_send_event(INITIAL_HANDSHAKE, send_initial_handshake);
    bind_recv_event(INITIAL_HANDSHAKE, recv_initial_handshake);

    bind_send_event(CLIENT_CONNECT, send_client_connect);
    bind_recv_event(CLIENT_CONNECT, recv_client_connect);
}
