/*
    pipenet.h

    NetEvent packet format:
        [protocol][args...]

        protocol: A NetProtocol enum. Will be used to write / read data in the specified format.
        args...: The raw bytes containing the relevant data of this event.

    Individual packet format:
        [packet_size][NetEvent]

        packet_size: A VLQ header. How large the following packet will be, in bytes. 4 bytes.

    Queued packet format:
        [packet_size][net_event_count][NetEvent...]

        packet_size: A VLQ header. How large the following packet will be, in bytes. 4 bytes.
        net_event_count: How many NetEvents are written to / to read from this queue. 4 bytes.

    Write process:
        Skip over the first sizeof(size_t) bytes of the NetBuffer (by writing 0).
        When we finish writing the rest of the event queue / event, write the NetBuffer's offset - sizeof(size_t) bytes to the header.

        Why the subtraction?
            To skip the sizeof(size_t) bytes counted in the NetBuffer's offset when we saved space at the start of the buffer.
            Our reported packet size would be sizeof(size_t) bytes greater than it really is.

        Now, our packet has an accurate VLQ header.

    Read process:
        First read sizeof(size_t) bytes. This is the size of the following packet.
        Then, allocate a buffer of the given size to fit all the data (e.g. char buffer[packet_size]).
        Use read to read the remainder of the packet into this buffer.
*/

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#ifndef PIPENET_H
#define PIPENET_H

#define NET_BUFFER_ALLOC(nb, alloc_size)                  \
    if ((nb)->offset + (alloc_size) > (nb)->size) {       \
        (nb)->size *= 2;                                  \
        (nb)->buffer = realloc((nb)->buffer, (nb)->size); \
    }

#define NET_BUFFER_WRITE(nb, ptr, size)                 \
    NET_BUFFER_ALLOC(nb, (size))                        \
    memcpy((nb)->buffer + (nb)->offset, (ptr), (size)); \
    (nb)->offset += (size);

#define NET_BUFFER_WRITE_VALUE(nb, value) \
    NET_BUFFER_WRITE((nb), &(value), sizeof((value)))

#define NET_BUFFER_BEGIN_WRITE(nb)              \
    {                                           \
        size_t packet_size = 0;                 \
        NET_BUFFER_WRITE_VALUE(nb, packet_size) \
    }

#define NET_BUFFER_END_WRITE(nb)                                 \
    {                                                            \
        size_t packet_size;                                      \
        packet_size = (nb)->offset - sizeof(packet_size);        \
        memcpy((nb)->buffer, &packet_size, sizeof(packet_size)); \
    }

// Write string length + string bytes
// Uses a scope so that len gets cleaned up
#define NET_BUFFER_WRITE_STRING(nb, string)   \
    {                                         \
        size_t len = strlen((string));        \
        NET_BUFFER_WRITE_VALUE((nb), len)     \
        NET_BUFFER_WRITE((nb), (string), len) \
    }

#define NET_BUFFER_READ(nb, ptr, size)                  \
    memcpy((ptr), (nb)->buffer + (nb)->offset, (size)); \
    (nb)->offset += (size);

#define NET_BUFFER_READ_VALUE(nb, var) \
    NET_BUFFER_READ((nb), &(var), sizeof(var))

//
// Uses a scope so that len gets cleaned up
#define NET_BUFFER_READ_STRING(nb, string)     \
    {                                          \
        size_t len;                            \
        NET_BUFFER_READ_VALUE((nb), (len))     \
        NET_BUFFER_READ((nb), (string), (len)) \
    }

typedef struct NetBuffer NetBuffer;
struct NetBuffer {
    size_t size;
    void *buffer;
    int offset;
};

typedef enum NetProtocol NetProtocol;
enum NetProtocol {
    PERIODIC_HANDSHAKE,
    INITIAL_HANDSHAKE,

    PROTOCOL_COUNT,
};

typedef void (*NetEventWriter)(NetBuffer *nb, void *args);
typedef void *(*NetEventReader)(NetBuffer *nb, void *args);

typedef struct NetEventHandler NetEventHandler;
struct NetEventHandler {
    NetProtocol protocol;

    NetEventWriter write_fn;
    NetEventReader read_fn;
};

typedef struct NetEvent NetEvent;
struct NetEvent {
    NetProtocol protocol;
    void *args;
};

typedef struct NetEventQueue NetEventQueue;
struct NetEventQueue {
    int event_count;
    int max_events;

    NetEvent **events;
};

NetEvent *net_event_new(NetProtocol protocol, void *args);

NetEventQueue *net_event_queue_new();

void insert_event(NetEventQueue *net_event_queue, NetEvent *event);

void empty_net_event_queue(NetEventQueue *net_event_queue);

void send_event_queue(NetEventQueue *net_event_queue, int send_fd);

void send_event_immediate(NetEvent *event, int send_fd);

void recv_event_queue(NetEventQueue *net_event_queue, void *recv_buffer);

NetEvent *recv_event_immediate(int recv_fd, NetEvent *recv_event);

void net_init();

#endif
