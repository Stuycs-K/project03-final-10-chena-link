#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#ifndef PIPERW_H
#define PIPERW_H

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

typedef enum NetProtocol NetProtocol;
enum NetProtocol {
    PERIODIC_HANDSHAKE,

    PROTOCOL_COUNT,
};

typedef struct NetBuffer NetBuffer;
struct NetBuffer {
    size_t size;
    void *buffer;
    int offset;
};

typedef void (*NetEventWriter)(NetBuffer *nb, void *args);
typedef int (*NetEventReader)(NetBuffer *nb);

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

NetEventQueue *net_event_queue_new();

void insert_event(NetEventQueue *net_event_queue, NetEvent *event);

void send_event_queue(NetEventQueue *net_event_queue, int send_fd);

void bind_send_event(NetProtocol protocol, NetEventWriter writer);

void bind_recv_event(NetProtocol protocol, NetEventReader reader);

void net_init();

#endif
