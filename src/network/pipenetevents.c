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

void send_initial_handshake(NetBuffer *nb, void *args) {
    NetArgs_InitialHandshake *nargs = args;

    NET_BUFFER_WRITE_VALUE(nb, nargs->syn_ack);
    NET_BUFFER_WRITE_VALUE(nb, nargs->ack);
    NET_BUFFER_WRITE_VALUE(nb, nargs->errcode);

    // NET_BUFFER_WRITE_VALUE(nb, nargs->client_to_server_fd);

    // NET_BUFFER_WRITE_VALUE(nb, nargs->server_to_client_fd);

    NET_BUFFER_WRITE_STRING(nb, nargs->to_client_pipe_name);

    NET_BUFFER_WRITE_VALUE(nb, nargs->client_id);
}

void *recv_initial_handshake(NetBuffer *nb, void *args) {
    NetArgs_InitialHandshake *nargs = args;

    NET_BUFFER_READ_VALUE(nb, nargs->syn_ack);
    NET_BUFFER_READ_VALUE(nb, nargs->ack);
    NET_BUFFER_READ_VALUE(nb, nargs->errcode);

    // NET_BUFFER_READ_VALUE(nb, nargs->client_to_server_fd);

    // NET_BUFFER_READ_VALUE(nb, nargs->server_to_client_fd);

    NET_BUFFER_READ_STRING(nb, nargs->to_client_pipe_name);
    NET_BUFFER_READ_VALUE(nb, nargs->client_id);

    return nargs;
}

void send_client_connect(NetBuffer *nb, void *args) {
    NetArgs_ClientConnect *nargs = args;

    NET_BUFFER_WRITE_STRING(nb, nargs->name);
}
void *recv_client_connect(NetBuffer *nb, void *args) {
    NetArgs_ClientConnect *nargs = args;

    NET_BUFFER_READ_STRING(nb, nargs->name);

    return nargs;
}
