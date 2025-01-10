#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file.h"

int set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl (F_GETFL)");
        return -1;
    }

    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) == -1) {
        perror("fcntl (F_SETFL)");
        return -1;
    }

    return 1;
}