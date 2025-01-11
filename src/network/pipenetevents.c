// Place send and recv handlers for each network protocol here.
// They should be named "send_[protocol name in lowercase]" and "recv_[protocol name in lowercase]"
// Then, bind them in net_init() in pipenet.c

#include <stdio.h>

#include "../shared.h"
#include "pipenetevents.h"

#define DECLARE_CONSTRUCTOR(net_args_type_name, internal_name) \
    net_args_type_name *(nargs_##internal_name)() {            \
        net_args_type_name *nargs = malloc(sizeof(net_args_type_name));

#define END_CONSTRUCTOR() \
    return nargs;         \
    }

#define DECLARE_HANDLER(net_args_type_name, internal_name)               \
    void *handler_##internal_name(NetBuffer *nb, void *args, int mode) { \
        if (args == NULL) {                                              \
            args = nargs_##internal_name();                              \
        }                                                                \
        net_args_type_name *nargs = args;

#define VALUE(field)                      \
    if (mode == 0) {                      \
        NET_BUFFER_WRITE_VALUE(nb, field) \
    } else {                              \
        NET_BUFFER_READ_VALUE(nb, field)  \
    }

#define PTR(field, size)                  \
    if (mode == 0) {                      \
        NET_BUFFER_WRITE(nb, field, size) \
    } else {                              \
        NET_BUFFER_READ(nb, field, size)  \
    }

#define STRING(field)                      \
    if (mode == 0) {                       \
        NET_BUFFER_WRITE_STRING(nb, field) \
    } else {                               \
        NET_BUFFER_READ_STRING(nb, field)  \
    }

#define END_HANDLER() \
    return nargs;     \
    }

DECLARE_CONSTRUCTOR(NetArgs_PeriodicHandshake, periodic_handshake) {
    nargs->id = 69420;
}
END_CONSTRUCTOR()

DECLARE_HANDLER(NetArgs_PeriodicHandshake, periodic_handshake) {
    VALUE(nargs->id);
}
END_HANDLER()

//============================================================

DECLARE_CONSTRUCTOR(NetArgs_Handshake, handshake) {
    nargs->ack = -1;
    nargs->errcode = -1;
    nargs->syn_ack = -1;
    nargs->are_fds_finalized = 0;
    nargs->client_to_server_fd = -1;
    nargs->server_to_client_fd = -1;
    nargs->client_id = -1;
    nargs->to_client_pipe_name = calloc(sizeof(char), 13);
}
END_CONSTRUCTOR()

DECLARE_HANDLER(NetArgs_Handshake, handshake) {
    VALUE(nargs->syn_ack);
    VALUE(nargs->ack);
    VALUE(nargs->errcode);
    VALUE(nargs->are_fds_finalized);

    if (nargs->are_fds_finalized) {
        VALUE(nargs->client_to_server_fd);
        VALUE(nargs->server_to_client_fd);
    }

    STRING(nargs->to_client_pipe_name);
    VALUE(nargs->client_id);
}
END_HANDLER()

void send_handshake(NetBuffer *nb, void *args) {
    NetArgs_Handshake *nargs = args;

    NET_BUFFER_WRITE_VALUE(nb, nargs->syn_ack);
    NET_BUFFER_WRITE_VALUE(nb, nargs->ack);
    NET_BUFFER_WRITE_VALUE(nb, nargs->errcode);

    NET_BUFFER_WRITE_VALUE(nb, nargs->are_fds_finalized);
    if (nargs->are_fds_finalized) {
        NET_BUFFER_WRITE_VALUE(nb, nargs->client_to_server_fd);
        NET_BUFFER_WRITE_VALUE(nb, nargs->server_to_client_fd);
    }

    NET_BUFFER_WRITE_STRING(nb, nargs->to_client_pipe_name);
    NET_BUFFER_WRITE_VALUE(nb, nargs->client_id);
}

void *recv_handshake(NetBuffer *nb, void *args) {
    if (args == NULL) {
        args = nargs_handshake();
    }

    NetArgs_Handshake *nargs = args;

    NET_BUFFER_READ_VALUE(nb, nargs->syn_ack);
    NET_BUFFER_READ_VALUE(nb, nargs->ack);
    NET_BUFFER_READ_VALUE(nb, nargs->errcode);

    NET_BUFFER_READ_VALUE(nb, nargs->are_fds_finalized);
    if (nargs->are_fds_finalized) {
        NET_BUFFER_READ_VALUE(nb, nargs->client_to_server_fd);
        NET_BUFFER_READ_VALUE(nb, nargs->server_to_client_fd);
    }

    NET_BUFFER_READ_STRING(nb, nargs->to_client_pipe_name);
    NET_BUFFER_READ_VALUE(nb, nargs->client_id);

    return nargs;
}

//============================================================

DECLARE_CONSTRUCTOR(NetArgs_ClientConnect, client_connect) {
    nargs->name = calloc(sizeof(char), 20);
}
END_CONSTRUCTOR()

DECLARE_HANDLER(NetArgs_ClientConnect, client_connect) {
    STRING(nargs->name);
    VALUE(nargs->to_client_fd);
}
END_HANDLER()

//============================================================
