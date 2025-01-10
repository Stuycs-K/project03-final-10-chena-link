#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../network/pipehandshake.h"
#include "../shared.h"
#include "subserver.h"

// Propagates client messages to the main server.
int propagate(Subserver *this) {
    int client_id = this->client_id;

    // First, get packet size
    size_t packet_size;
    ssize_t bytes_read = read(this->recv_fd, &packet_size, sizeof(packet_size));

    if (bytes_read <= 0) {
        if (bytes_read == 0) {
            // CLIENT DISCONNECT! Send message
            return -1;
        }
    }

    size_t total_size = packet_size + sizeof(packet_size) + sizeof(client_id);

    char *raw_recv_buffer = malloc(sizeof(char) * total_size); // Will be sent to the main server

    int offset = 0;
    memcpy(raw_recv_buffer + offset, &client_id, sizeof(client_id)); // Write client ID so the main server knows who sent these messages
    offset += sizeof(client_id);

    memcpy(raw_recv_buffer + offset, &packet_size, sizeof(packet_size)); // And then copy over the packet size
    offset += sizeof(packet_size);

    bytes_read = read(this->recv_fd, raw_recv_buffer + offset, packet_size); // And finally, read the rest of the packet directly into the buffer

    ssize_t bytes_written = write(this->main_pipe[PIPE_WRITE], raw_recv_buffer, total_size);

    if (bytes_written <= 0) {
        free(raw_recv_buffer);
        return -1;
    }

    free(raw_recv_buffer);
    return 1;
}

Subserver *subserver_new(int client_id) {
    Subserver *this = malloc(sizeof(Subserver));

    this->client_id = client_id;
    this->send_fd = -1;
    this->recv_fd = -1;
    this->pid = -1;

    this->handshake_event = NULL;

    return this;
}

int subserver_is_inactive(Subserver *this) {
    return (this->pid == -1 && this->recv_fd == -1);
}

void subserver_run(Subserver *this) {
    close(this->main_pipe[PIPE_READ]);

    this->pid = getpid();

    // ACK
    int status = server_complete_handshake(this->handshake_event);

    free_handshake_event(this->handshake_event);
    this->handshake_event = NULL;

    if (status == -1) {
        printf("ACK Fail\n");
        exit(EXIT_FAILURE);
    }

    printf("CONNECTION MADE WITH CLIENT!\n");

    // Read loop
    // The subserver propogates the data to the main server through its pipe.
    while (1) {
        if (propagate(this) == -1) {
            break;
        }
    }

    printf("CLIENT DISCONNECT\n");

    exit(EXIT_SUCCESS);
}