#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "piperw.h"

// Holy hell
void recv_event(int recv_fd) {
    char recv_buffer[2048];

    void *args = malloc(sizeof(recv_buffer) - sizeof(NetProtocol));

    ssize_t bytes_read = read(recv_fd, &recv_buffer, sizeof(recv_buffer));
    if (bytes_read <= 0) {
        return;
    }

    // Read protocol
    NetProtocol protocol;
    memcpy(&protocol, (void *)recv_buffer, sizeof(NetProtocol));

    // Read the rest of the data into the args buffer
    memcpy(args, (void *)recv_buffer + sizeof(NetProtocol), sizeof(recv_buffer) - sizeof(NetProtocol));
}

// Events are sent with a protocol header, defined in the NetProtocol enum.
void send_event(int send_fd, NetProtocol protocol, void *args) {
    void *send_buffer = malloc(sizeof(args) + sizeof(protocol));
    memcpy(send_buffer, &protocol, sizeof(protocol));
    memcpy(send_buffer + sizeof(protocol), args, sizeof(args));

    ssize_t bytes_written = write(send_fd, send_buffer, sizeof(send_buffer));
    if (bytes_written <= 0) {
        // Do something
    }

    free(send_buffer);
}

void bind_send_event(NetProtocol protocol, int (*write_fn)(int, void *, void *)) {
}

void bind_recv_event(NetProtocol protocol, int (*read_fn)(int, void *, void *)) {
}