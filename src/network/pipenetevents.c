// Place send and recv handlers for each network protocol here.
// They should be named "send_[protocol name in lowercase]" and "recv_[protocol name in lowercase]"
// Then, bind them in net_init() in pipenet.c

#include <stdio.h>

#include "pipenetevents.h"

void send_periodic_handshake(NetBuffer *nb, void *args) {
    NetArgs_PeriodicHandshake *nargs = (NetArgs_PeriodicHandshake *)args;
    NET_BUFFER_WRITE_VALUE(nb, nargs->id);
}

void *recv_periodic_handshake(NetBuffer *nb, void *args) {
    NetArgs_PeriodicHandshake *nargs = malloc(sizeof(NetArgs_PeriodicHandshake));
    NET_BUFFER_READ_VALUE(nb, nargs->id);

    return nargs;
}

NetArgs_InitialHandshake *nargs_initial_handshake() {
    NetArgs_InitialHandshake *nargs = malloc(sizeof(NetArgs_InitialHandshake));

    nargs->ack = -1;
    nargs->errcode = -1;
    nargs->syn_ack = -1;
    nargs->to_client_pipe_name = calloc(sizeof(char), 12);

    return nargs;
}

void send_initial_handshake(NetBuffer *nb, void *args) {
    NetArgs_InitialHandshake *nargs = args;

    NET_BUFFER_WRITE_VALUE(nb, nargs->syn_ack);
    NET_BUFFER_WRITE_VALUE(nb, nargs->ack);
    NET_BUFFER_WRITE_VALUE(nb, nargs->errcode);
    NET_BUFFER_WRITE_STRING(nb, nargs->to_client_pipe_name);
}

void *recv_initial_handshake(NetBuffer *nb, void *args) {
    NetArgs_InitialHandshake *nargs = args;

    NET_BUFFER_READ_VALUE(nb, nargs->syn_ack);
    NET_BUFFER_READ_VALUE(nb, nargs->ack);
    NET_BUFFER_READ_VALUE(nb, nargs->errcode);
    NET_BUFFER_READ_STRING(nb, nargs->to_client_pipe_name);

    return nargs;
}