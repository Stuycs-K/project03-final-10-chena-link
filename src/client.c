#include "client.h"
#include "network/pipehandshake.h"

void client_main(void) {
    int net_fds[2];
    client_handshake("TEMP", net_fds);
}