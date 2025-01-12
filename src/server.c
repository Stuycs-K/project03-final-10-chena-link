#include "server.h"
#include "cserver.h"
#include "gserver.h"

void server_main() {
    CServer *cserver = cserver_new(1);
    cserver_run(cserver);

    // GServer *gserver = gserver_new(69);

    // gserver_run(gserver);
}