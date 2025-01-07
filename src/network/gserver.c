#include <stdlib.h>
#include <unistd.h>

#include "gserver.h"

char *get_client_to_server_fifo_name() {
    // return getpid();
    return 100;
}