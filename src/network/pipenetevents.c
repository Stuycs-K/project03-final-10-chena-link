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