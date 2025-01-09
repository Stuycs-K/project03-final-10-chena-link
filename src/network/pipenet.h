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

        Now, our packet has an accurate VLQ header and is ready to send.

    Read process:
        First read sizeof(size_t) bytes. This is the size of the following packet.
        Then, allocate a buffer of the given size to fit all the data (e.g. char buffer[packet_size]).
        Use read to read the remainder of the packet into this buffer.

        Finally, process the packet, translating the raw bytes into NetEvents.
*/

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#ifndef PIPENET_H
#define PIPENET_H

/*
    NET_BUFFER_ALLOC

    This is a function-like macro!
    This is a helper macro and should NEVER be used externally!

    Resizes nb->buffer using a growth factor of 2 if writing alloc_size bytes
    would cause overflow.

    NetBuffer *nb : the NetBuffer
    size_t alloc_size : how many bytes to allocate
*/
#define NET_BUFFER_ALLOC(nb, alloc_size)                  \
    if ((nb)->offset + (alloc_size) > (nb)->size) {       \
        (nb)->size *= 2;                                  \
        (nb)->buffer = realloc((nb)->buffer, (nb)->size); \
    }

/*
    NET_BUFFER_WRITE

    This is a function-like macro!

    Writes size bytes of ptr to nb->buffer. Checks for resizing and updates offset.

    NetBuffer *nb : the NetBuffer
    void *ptr : the pointer to data to write
    size_t size : how many bytes of data to write
*/
#define NET_BUFFER_WRITE(nb, ptr, size)                 \
    NET_BUFFER_ALLOC(nb, (size))                        \
    memcpy((nb)->buffer + (nb)->offset, (ptr), (size)); \
    (nb)->offset += (size);

/*
    NET_BUFFER_WRITE_VALUE

    This is a function-like macro!

    Wraps NET_BUFFER_WRITE. Used to write non-pointer values.

    NetBuffer *nb : the NetBuffer
    value : any non-pointer basic data type (e.g. char, int, size_t)
*/
#define NET_BUFFER_WRITE_VALUE(nb, value) \
    NET_BUFFER_WRITE((nb), &(value), sizeof((value)))

/*
    NET_BUFFER_BEGIN_WRITE

    This is a function-like macro!
    This is a helper macro and should NEVER be used externally!

    Reserves sizeof(size_t) bytes at the beginning of nb->buffer to record the packet size
    Called before writing anything else.

    NetBuffer *nb : the NetBuffer
*/
#define NET_BUFFER_BEGIN_WRITE(nb)                \
    {                                             \
        size_t packet_size = 0;                   \
        NET_BUFFER_WRITE_VALUE((nb), packet_size) \
    };

/*
    NET_BUFFER_BEGIN_WRITE

    This is a function-like macro!
    This is a helper macro and should NEVER be used externally!

    Writes the total packet size to the space reserved by NET_BUFFER_BEGIN_WRITE.

    NetBuffer *nb : the NetBuffer
*/
#define NET_BUFFER_END_WRITE(nb)                                 \
    {                                                            \
        size_t packet_size = 0;                                  \
        packet_size = (nb)->offset - sizeof(packet_size);        \
        memcpy((nb)->buffer, &packet_size, sizeof(packet_size)); \
    };

/*
    NET_BUFFER_WRITE_STRING

    This is a function-like macro!

    Writes a string to nb->buffer.
    First writes strlen(string), then the raw bytes of the string.

    NetBuffer *nb : the NetBuffer
    char *string : a string
*/
#define NET_BUFFER_WRITE_STRING(nb, string)   \
    {                                         \
        size_t len = strlen((string));        \
        NET_BUFFER_WRITE_VALUE((nb), len)     \
        NET_BUFFER_WRITE((nb), (string), len) \
    };

/*
    NET_BUFFER_READ

    This is a function-like macro!

    Reads size bytes from nb->buffer into ptr.

    NetBuffer *nb : the NetBuffer
    void *ptr : the pointer to the data to write to
    size_t size : how many bytes of data to read
*/
#define NET_BUFFER_READ(nb, ptr, size)                  \
    memcpy((ptr), (nb)->buffer + (nb)->offset, (size)); \
    (nb)->offset += (size);

/*
    NET_BUFFER_READ_VALUE

    This is a function-like macro!

    Wraps NET_BUFFER_READ. Used to read to non-pointer values

    NetBuffer *nb : the NetBuffer
    var : any non-pointer basic data type
*/
#define NET_BUFFER_READ_VALUE(nb, var) \
    NET_BUFFER_READ((nb), &(var), sizeof(var))

/*
    NET_BUFFER_READ_STRING

    This is a function-like macro!

    Reads a string from nb->buffer.
    First reads strlen(string). Then reads strlen(string) bytes.

    NetBuffer *nb : the NetBuffer
    char *string : a string
*/
#define NET_BUFFER_READ_STRING(nb, string)     \
    {                                          \
        size_t len;                            \
        NET_BUFFER_READ_VALUE((nb), (len))     \
        NET_BUFFER_READ((nb), (string), (len)) \
    };

/*
    size: The current allocated size of buffer, in bytes
    buffer: The buffer used to write data into / read data from
    offset: Where in the buffer we currently are
*/
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
    CLIENT_CONNECT,
    CLIENT_DISCONNECT,

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

void *read_into_buffer(int recv_fd);

void recv_event_queue(NetEventQueue *net_event_queue, void *recv_buffer);

NetEvent *recv_event_immediate(int recv_fd, NetEvent *recv_event);

void net_init();

#endif
