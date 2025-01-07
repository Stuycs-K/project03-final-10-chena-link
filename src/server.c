#include "server.h"
#include "network/gserver.h"

void server_main() {
    GServer *gserver = gserver_new();

    gserver_init(gserver);
}