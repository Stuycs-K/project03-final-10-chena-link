#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "client.h"
#include "cserver.h"

#include "network/pipenet.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        exit(EXIT_FAILURE);
    }

    net_init();

    char *mode = argv[1];
    if (!strcmp(mode, "client")) {
        client_main();
    } else if (!strcmp(mode, "gserver")) {
        CServer *cserver = cserver_new(1);
        printf("CServer initialized\n");

        cserver_run(cserver);
    }

    return 0;
}
