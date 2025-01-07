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
    NET_BUFFER_ALLOC(nb, size)                          \
    memcpy((nb)->buffer + (nb)->offset, (ptr), (size)); \
    (nb)->offset += (size);

#define NET_BUFFER_WRITE_VALUE(nb, value) \
    NET_BUFFER_WRITE((nb), &(value), sizeof((value)))

#define NET_BUFFER_READ(nb, ptr, size)                  \
    memcpy((ptr), (nb)->buffer + (nb)->offset, (size)); \
    (nb)->offset += (size);

#define NET_BUFFER_READ_VALUE(nb, var) \
    NET_BUFFER_READ((nb), &(var), sizeof(var))

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
typedef void *(*NetEventReader)(NetBuffer *nb);

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

extern NetEventHandler *g_net_event_handlers[];

NetEvent *net_event_new(NetProtocol protocol, void *args);

NetEventQueue *net_event_queue_new();

void insert_event(NetEventQueue *net_event_queue, NetEvent *event);

void empty_net_event_queue(NetEventQueue *net_event_queue);

void send_event_queue(NetEventQueue *net_event_queue, int send_fd);

void recv_event_queue(NetEventQueue *net_event_queue, void *recv_buffer);

void net_init();

#endif
