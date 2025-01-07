#include "server.h"
#include "network/gserver.h"
#include "network/pipenet.h"

void server_main() {
    net_init();
    GServer *gserver = gserver_new();

    gserver_init(gserver);
}