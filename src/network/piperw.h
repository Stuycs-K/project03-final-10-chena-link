#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#ifndef PIPERW_H
#define PIPERW_H

#define ALLOCATE(size) \
    if (offset + (size) > current_send_buf_size) {              \
        current_send_buf_size *= 2;                                \
        send_buffer = realloc(send_buffer, current_send_buf_size); \
    }

#define NET_BEGIN_FN() \
    int offset = *p_offset; \
    void *send_buffer = *p_send_buffer; \
    size_t current_send_buf_size = *p_size;

#define NET_END_FN() \
    *p_offset = offset; \
    *p_send_buffer = send_buffer; \
    *p_size = current_send_buf_size;

#define NET_BEGIN_SEND_BUFFER()                            \
    size_t current_send_buf_size = sizeof(char) * 512; \
    void *send_buffer = malloc(current_send_buf_size); \
    int offset = 0;

#define NET_SEND_VALUE(data)                                                 \
    size_t data_size = sizeof((data));                             \
    ALLOCATE(data_size)                                            \
    memcpy(send_buffer + offset, &(data), data_size);               \
    offset += data_size;

#define NET_SEND_PTR(ptr, size) \
    ALLOCATE(size) \
    memcpy(send_buffer + offset, (ptr), (size));               \
    offset += (size); \

#define NET_TRANSMIT_SEND_BUFFER(send_fd) \
    write((send_fd), send_buffer, offset);

#define NET_END_SEND_BUFFER() \
    free(send_buffer);

typedef enum NetProtocol NetProtocol;
enum NetProtocol {
    PERIODIC_HANDSHAKE,

    PROTOCOL_COUNT,
};

// Returns the size of data written
typedef void (*NetEventWriter)(void *args, void **p_send_buffer, int *p_offset, size_t *p_size);
typedef int (*NetEventReader)(void *recv);

typedef struct NetBuffer NetBuffer;
struct NetBuffer {
    size_t size;
    void *buffer;
    int offset;
};

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
