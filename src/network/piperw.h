#ifndef PIPERW_H
#define PIPERW_H

typedef enum {
    PERIODIC_HANDSHAKE
} NetProtocol;

typedef struct NetEvent NetEvent;
struct NetEvent {
    NetProtocol protocol;
    void *args;

    int (*write_fn)(int, void *, void *);
    int (*read_fn)(int, void *, void *)
};

void recv_event(int recv_fd);

void send_event(int send_fd, NetProtocol protocol, void *args);

#endif