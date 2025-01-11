/*
    For each protocol:

    Build a constructor function with DECLARE_CONSTRUCTOR. Make sure to close it with END_CONSTRUCTOR
    Build a handler function with DECLARE_HANDLER. Make sure to close it with END_HANDLER
*/

#include <stdio.h>

#include "../shared.h"
#include "pipenetevents.h"

// The variable "nargs" is available and has the correct type
#define DECLARE_CONSTRUCTOR(net_args_type_name, internal_name) \
    net_args_type_name *(nargs_##internal_name)() {            \
        net_args_type_name *nargs = malloc(sizeof(net_args_type_name));

#define END_CONSTRUCTOR() \
    return nargs;         \
    }

// The variable "nargs" is available and has the correct type
#define DECLARE_HANDLER(net_args_type_name, internal_name)               \
    void *handler_##internal_name(NetBuffer *nb, void *args, int mode) { \
        if (args == NULL) {                                              \
            args = nargs_##internal_name();                              \
        }                                                                \
        net_args_type_name *nargs = args;

// Read / write a non-pointer field
#define VALUE(field)                      \
    if (mode == 0) {                      \
        NET_BUFFER_WRITE_VALUE(nb, field) \
    } else {                              \
        NET_BUFFER_READ_VALUE(nb, field)  \
    }

// Read / write a pointer field (needs size)
#define PTR(field, size)                  \
    if (mode == 0) {                      \
        NET_BUFFER_WRITE(nb, field, size) \
    } else {                              \
        NET_BUFFER_READ(nb, field, size)  \
    }

// Read / write a string (char *) field
#define STRING(field)                      \
    if (mode == 0) {                       \
        NET_BUFFER_WRITE_STRING(nb, field) \
    } else {                               \
        NET_BUFFER_READ_STRING(nb, field)  \
    }

#define END_HANDLER() \
    return nargs;     \
    }

//============================================================

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
