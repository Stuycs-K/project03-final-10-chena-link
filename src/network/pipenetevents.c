// Place send and recv handlers for each network protocol here.
// They should be named "send_[protocol name in lowercase]" and "recv_[protocol name in lowercase]"
// Then, bind them in net_init() in pipenet.c

#include <stdio.h>

#include "pipenetevents.h"

void send_periodic_handshake(NetBuffer *nb, void *args) {
    NetArgs_PeriodicHandshake *nargs = (NetArgs_PeriodicHandshake *)args;
    NET_BUFFER_WRITE_VALUE(nb, nargs->id);
}

void *recv_periodic_handshake(NetBuffer *nb) {
    NetArgs_PeriodicHandshake *nargs = malloc(sizeof(NetArgs_PeriodicHandshake));
    NET_BUFFER_READ_VALUE(nb, nargs->id);

    return nargs;
}