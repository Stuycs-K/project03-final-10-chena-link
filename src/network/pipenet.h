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

NetEvent *recv_event_immediate(void *recv_buffer, NetEvent *recv_event);

void net_init();

#endif
