#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "debug.h"

void fatal_assert(int cond, char *message, ...) {
    va_list args;
    va_start(args, message);

    if (cond == -1) {
        vfprintf(stderr, message, args);
        exit(EXIT_FAILURE);
    }

    va_end(args);
}