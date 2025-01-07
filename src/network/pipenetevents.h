#include "pipenet.h"

#ifndef PIPENETEVENTS_H
#define PIPENETEVENTS_H

typedef struct NetArgs_PeriodicHandshake NetArgs_PeriodicHandshake;
struct NetArgs_PeriodicHandshake {
    int id;
};

void send_periodic_handshake(NetBuffer *nb, void *args);
void *recv_periodic_handshake(NetBuffer *nb);

#endif