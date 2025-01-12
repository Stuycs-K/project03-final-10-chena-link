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
    This is a function-like macro!

    Wraps NET_BUFFER_WRITE. Used to write non-pointer values.

    NetBuffer *nb : the NetBuffer
    value : any non-pointer basic data type (e.g. char, int, size_t)
*/
#define NET_BUFFER_WRITE_VALUE(nb, value) \
    NET_BUFFER_WRITE((nb), &(value), sizeof((value)))

/*
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
    This is a function-like macro!

    Wraps NET_BUFFER_READ. Used to read to non-pointer values

    NetBuffer *nb : the NetBuffer
    var : any non-pointer basic data type
*/
#define NET_BUFFER_READ_VALUE(nb, var) \
    NET_BUFFER_READ((nb), &(var), sizeof(var))

/*
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
    Used to compile all the data you need to write / receive into a single buffer
    The batched data can then be atomically written to a file (by atomic I mean with a single write call), or deserialized "gracefully" enough

    size_t size : The current allocated size of buffer, in bytes
    void *buffer : The buffer used to write data into / read data from
    int offset : Where in the buffer we currently are
*/
typedef struct NetBuffer NetBuffer;
struct NetBuffer {
    size_t size;
    void *buffer;
    int offset;
};

/*
    Describes the type of NetEvent.
    Used to determine which NetEventHandler to use to read / write the event.
*/
typedef enum NetProtocol NetProtocol;
enum NetProtocol {
    PERIODIC_HANDSHAKE,
    HANDSHAKE,
    CLIENT_LIST,
    GSERVER_INFO,
    GSERVER_LIST,

    PROTOCOL_COUNT,
};

/*
    Don't worry about calling this. It's handled automagically.
    Reads / writes an event of a specific protocol. Composed with DECLARE_HANDLER in pipenetevents.c

    NetBuffer *nb : the NetBuffer
    void *args: the arguments of a NetEvent
    int mode : 0 is write mode, 1 is read mode
*/
typedef void *(*NetEventHandler)(NetBuffer *nb, void *args, int mode);

/*
    Don't worry about calling this. It's handled automagically.
    Frees the argument struct of a NetEvent. Composed with DECLARE_DESTRUCTOR in pipenetevents.c

    void *args: the arguments of a NetEvent that this will free
*/
typedef void (*NetEventDestructor)(void *args);

/*
    When NetEventQueues are emptied, they have a few options to cleanup the NetEvents:

    NEVENT_BASE: the NetEvent and its args will be freed (default)
    NEVENT_PERSISTENT_ARGS: the NetEvent will be freed but the args will not
    NEVENT_PERSISTENT: nothing will be freed
*/
typedef enum NetEventCleanupBehavior NetEventCleanupBehavior;
enum NetEventCleanupBehavior {
    NEVENT_BASE,
    NEVENT_PERSISTENT_ARGS,
    NEVENT_PERSISTENT
};

/*
    NetEvents wrap networked structs for networking.

    NetProtocol protocol : see NetProtocol

    NetEventCleanupBehavior cleanup_behavior : see NetEventCleanupBehavior

    void *args : a struct of data linked to the NetProtocol
        Must be cast into the correct NetArgs object with a statement such as NetArgs_EventName *nargs = event->args;
        Can be freely manipulated from that point onward.
*/
typedef struct NetEvent NetEvent;
struct NetEvent {
    NetProtocol protocol;
    NetEventCleanupBehavior cleanup_behavior;
    void *args;
};

/*
    NetEventQueues batch NetEvents together for a singular read / write operation after some time.

    int event_count : how many NetEvents are being sent / received
    int max_events : the maximum size of the queue

    NetEvent **attached_events :

    NetEvent **events : a contiguous array of NetEvents
*/
typedef struct NetEventQueue NetEventQueue;
struct NetEventQueue {
    int event_count;
    int max_events;

    NetEvent **attached_events;
    NetEvent **events;
};

NetEvent *net_event_new(NetProtocol protocol, void *args);

NetEventQueue *net_event_queue_new();

void insert_event(NetEventQueue *net_event_queue, NetEvent *event);

void attach_event(NetEventQueue *net_event_queue, NetEvent *event);

void detach_event(NetEventQueue *net_event_queue, NetEvent *event);

void empty_net_event_queue(NetEventQueue *net_event_queue);

void send_event_queue(NetEventQueue *net_event_queue, int send_fd);

void send_event_immediate(NetEvent *event, int send_fd);

void *read_into_buffer(int recv_fd);

void recv_event_queue(NetEventQueue *net_event_queue, void *recv_buffer);

NetEvent *recv_event_immediate(int recv_fd, NetEvent *recv_event);

void net_init();

#endif
