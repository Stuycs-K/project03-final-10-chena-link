#include <fcntl.h>
#include <unistd.h>

#include "client.h"
#include "network/pipehandshake.h"
#include "network/piperw.h"

void client_main(void) {
    int net_fds[2];
    client_handshake("TEMP", net_fds);
}