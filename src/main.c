#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "client.h"
#include "debug/debug.h"
#include "server.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        exit(EXIT_FAILURE);
    }

    char *mode = argv[1];
    if (!strcmp(mode, "client")) {
        printf("client\n");
        client_main();
    } else if (!strcmp(mode, "server")) {
        printf("server\n");
        server_main();
    }
}
