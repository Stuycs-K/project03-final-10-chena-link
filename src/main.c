#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug/debug.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        exit(EXIT_FAILURE);
    }

    char *mode = argv[1];
    if (!strcmp(mode, "client")) {
        printf("client\n");
    } else if (!strcmp(mode, "server")) {
        printf("server\n");
    }

    fatal_assert(-1, "Yikes %d\n", 100);
}
