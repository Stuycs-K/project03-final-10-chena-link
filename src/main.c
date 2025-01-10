#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "client.h"
#include "debug/debug.h"
#include "server.h"

#include "network/pipenet.h"
#include "testserver.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        exit(EXIT_FAILURE);
    }

    net_init();

    char *mode = argv[1];
    if (!strcmp(mode, "client")) {
        printf("client\n");
        client_main();
    } else if (!strcmp(mode, "gserver")) {
        printf("server\n");
        test_main();
    } else if (!strcmp(mode, "cserver")) {
        printf("cserver\n");
    }
}
