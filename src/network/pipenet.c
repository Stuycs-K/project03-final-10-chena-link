#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "pipenet.h"
#include "pipenetevents.h"

/*
    Used to bind the necessary functions for each protocol

    protocol : the NetProtocol
    internal_name : the same token you used to define constructors, handlers and destructors in pipenetevents.h and .c
*/
#define BIND(protocol, internal_name)             \
    handlers[protocol] = handler_##internal_name; \
    destructors[protocol] = destroy_##internal_name;

NetEventHandler handlers[PROTOCOL_COUNT];
NetEventDestructor destructors[PROTOCOL_COUNT];

/*
    Binds all protocols to their handlers. Called once in main.c, and that's it.

    PARAMS: none

    RETURNS: none
*/
void net_init() {
    BIND(HANDSHAKE, handshake);
    BIND(CLIENT_LIST, client_list);
    BIND(GSERVER_INFO, gserver_info);
    BIND(GSERVER_LIST, gserver_info_list);
    BIND(RESERVE_GSERVER, reserve_gserver);
    BIND(CARD_COUNT, card_count_array);
    BIND(SHMID, shmid);
    BIND(GSERVER_CONFIG, gserver_config);
}

/*
    Create a NetBuffer struct meant for writing to.
    The default size of the internal buffer is 512 bytes.

    PARAMS: none

    RETURNS: the created NetBuffer
*/
static NetBuffer *net_buffer_send() {
    NetBuffer *new_net_buffer = malloc(sizeof(NetBuffer));

    new_net_buffer->size = 512;
    new_net_buffer->buffer = malloc(sizeof(char) * 512);
    new_net_buffer->offset = 0;

    return new_net_buffer;
}

/*
    Loads a buffer into a NetBuffer struct.
    Only use for reading operations.

    PARAMS:
        void *buffer : the data to read

    RETURNS: the created NetBuffer
*/
static NetBuffer *net_buffer_recv(void *buffer) {
    NetBuffer *new_net_buffer = malloc(sizeof(NetBuffer));

    new_net_buffer->size = 0;
    new_net_buffer->buffer = buffer;
    new_net_buffer->offset = 0;

    return new_net_buffer;
}

/*
    Loads a buffer into a NetBuffer struct.
    Only use for reading operations.

    PARAMS:
        void *buffer : the data to read

    RETURNS: the created NetBuffer
*/
static void transmit_net_buffer(NetBuffer *net_buffer, int target_fd) {
    ssize_t bytes_written = write(target_fd, net_buffer->buffer, net_buffer->offset);
    if (bytes_written <= 0) {
        perror("transmit_net_buffer");
    }
}

/*
    Frees the NetBuffer's internal buffer and the NetBuffer struct

    PARAMS:
        NetBuffer *net_buffer : the NetBuffer to clean up

    RETURNS: none
*/
static void free_net_buffer(NetBuffer *net_buffer) {
    free(net_buffer->buffer);
    free(net_buffer);
}

/*
    Create a NetEvent to wrap around some data. This NetEvent can be passed into a send function to transmit it

    PARAMS:
        NetProtocol protocol : what "type" of data args is, so we know exactly how to read / write.
        void *args : the data we're trying to send. Should be a struct

    RETURNS: the created NetEvent
*/
NetEvent *net_event_new(NetProtocol protocol, void *args) {
    NetEvent *net_event = malloc(sizeof(NetEvent));
    net_event->protocol = protocol;
    net_event->args = args;
    net_event->cleanup_behavior = NEVENT_BASE;
    return net_event;
}

/*
    Create a NetEventQueue to batch together NetEvents (in the events field) to send OR to read raw data all at once and
    generate NetEvents (in the events field)

    PARAMS: none

    RETURNS: the created NetEventQueue
*/
NetEventQueue *net_event_queue_new() {
    NetEventQueue *net_event_queue = malloc(sizeof(NetEventQueue));

    net_event_queue->event_count = 0;
    net_event_queue->max_events = 255;

    net_event_queue->events = malloc(sizeof(NetEvent *) * net_event_queue->max_events);
    net_event_queue->attached_events = calloc(sizeof(NetEvent *), PROTOCOL_COUNT);

    return net_event_queue;
}

/*
    Inserts a NetEvent to the NetEventQueue. Use only for sending the NetEvent.

    PARAMS:
        NetEventQueue *net_event_queue : the NetEventQueue
        NetEvent *event : the NetEvent to add to the queue

    RETURNS: none
*/
void insert_event(NetEventQueue *net_event_queue, NetEvent *event) {
    if (net_event_queue->event_count >= net_event_queue->max_events) {
        return;
    }

    net_event_queue->events[net_event_queue->event_count++] = event;
}

/*
    Add a NetEvent to the attached events list and sets its cleanup behavior to be persistent.
    Attached events aren't removed in clear_event_queue, so you must use detach_event.
    There can only be one attached NetEvent per protocol.

    Useful for reusing a NetEvent.

    FOR SENDING:
        Attached events will be automatically inserted into the queue before writing.
    FOR RECEIVING:
        If we read event data with a protocol that matches the attached event,
        the rest of the data will be written into the attached event's args.

        No new NetEvent will be created as a result. The attached event will still
        be added to the queue.

    PARAMS:
        NetEventQueue *net_event_queue : the NetEventQueue
        NetEvent *event : the NetEvent to attach

    RETURNS: none
*/
void attach_event(NetEventQueue *net_event_queue, NetEvent *event) {
    event->cleanup_behavior = NEVENT_PERSISTENT;
    net_event_queue->attached_events[event->protocol] = event;
}

/*
    Removes a NetEvent from the attached events list.

    PARAMS:
        NetEventQueue *net_event_queue : the NetEventQueue
        NetEvent *event : the NetEvent to detach

    RETURNS: none
*/
void detach_event(NetEventQueue *net_event_queue, NetEvent *event) {
    net_event_queue->attached_events[event->protocol] = NULL;
}

/*
    Clears all NetEvents from the NetEventQueue.
    NetEvents are cleaned up according to their cleanup_behavior.
    By default, they (and their args data) will be freed.

    Call after send_event_queue and before recv_event_queue.

    PARAMS:
        NetEventQueue *net_event_queue : the NetEventQueue

    RETURNS: none
*/
void clear_event_queue(NetEventQueue *net_event_queue) {
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

/*
    Writes all events in the NetEventQueue to a NetBuffer (freed at the end).
    Then, the contents of the NetBuffer are written to the file descriptor.

    PARAMS:
        NetEventQueue *net_event_queue : the NetEventQueue
        int send_fd : which file to send the data to

    RETURNS: none
*/
void send_event_queue(NetEventQueue *net_event_queue, int send_fd) {
    // Add attached events
    for (int i = 0; i < PROTOCOL_COUNT; ++i) {
        NetEvent *attached_event = net_event_queue->attached_events[i];
        if (attached_event != NULL) {
            insert_event(net_event_queue, attached_event);
        }
    }

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

        NET_BUFFER_WRITE_VALUE(nb, protocol) // Write the event's protocol
        handlers[protocol](nb, args, 0);
    }

    NET_BUFFER_END_WRITE(nb)

    transmit_net_buffer(nb, send_fd);
    free_net_buffer(nb);
}

/*
    Serializes and sends ONE event immediately to the given file descriptor.
    The NetEvent is not freed after sending.
    Used for handshakes.

    The packet structure written will conform to that of queued data, so receiving data using recv_event_queue
    sent by send_event_immediate will work. However, it's suggested to use recv_event_immediate instead to receive data sent by this.

    PARAMS:
        NetEvent *event : the NetEvent to send
        int send_fd : which file to send the data to

    RETURNS: none
*/
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

/*
    A utility function that extracts (does not serialize) a single packet from the file descriptor.
    It first reads the VLQ (the packet size, in bytes) and then the amount of bytes specified into a malloc'd buffer.

    If you are using recv_event_queue with this, you don't have to free the buffer returned by this.

    PARAMS:
        NetEvent *event : the NetEvent to send
        int send_fd : which file to send the data to

    RETURNS: the malloc'd buffer
*/
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

/*
    Deserializes data stored in recv_buffer into NetEvents, which are stored in the NetEventQueue's events.
    The recv_buffer is fed into a NetBuffer (freed at the end), which is then processed to generate all NetEvents.

    PARAMS:
        NetEvent *event : the NetEvent to send
        void *recv_buffer : the buffer to read data from. Use read_into_buffer to get this.

    RETURNS: none
*/
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
            insert_event(net_event_queue, attached_event);
            continue;
        }

        void *data = handlers[protocol](nb, NULL, 1);
        insert_event(net_event_queue, net_event_new(protocol, data));
    }

    free_net_buffer(nb);
}

/*
    The companion to send_event_immediate. Reads and deserializes ONE NetEvent from the given file descriptor.
    Used for handshakes.

    PARAMS:
        int recv_fd : which file to receive the data from
        NetEvent *recv_event : if NULL, this mallocs a NetEvent to write into. If provided, the data will be written
        into the provided event's args.

    RETURNS: the deserialized NetEvent
*/
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
