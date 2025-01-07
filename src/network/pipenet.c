#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define NET_DEBUG

#include "pipenet.h"

int server_setup(char *client_to_server_fifo) {
    remove(client_to_server_fifo);

    // Make named pipe
    int mkfifo_ret = mkfifo(client_to_server_fifo, 0644);

    printf("[SERVER]: Created WKP\n");
    printf("[SERVER]: Waiting for connection...\n");

    int from_client = open(client_to_server_fifo, O_RDONLY, 0);

    printf("[SERVER]: Client connected\n");

    remove(client_to_server_fifo);

    return from_client;
}

int server_handshake(int from_client) {
    printf("[SERVER]: Waiting for SYN...\n");

    char syn[HANDSHAKE_BUFFER_SIZE];
    read(from_client, syn, sizeof(syn));

    printf("[SERVER]: Received SYN: %s\n", syn);

    int downstream = open(syn, O_WRONLY, 0);
    ASSERT(downstream, "Server open PP")

    int syn_ack_value = rand();
    ssize_t bytes = write(downstream, &syn_ack_value, sizeof(syn_ack_value));
    ASSERT(bytes, "Server write SYN-ACK")

    printf("[SERVER]: Sent SYN-ACK: %d\n", syn_ack_value);
    printf("[SERVER]: Waiting for ACK...\n");

    int ack_value;
    bytes = read(from_client, &ack_value, sizeof(ack_value));
    ASSERT(bytes, "Server read ACK")

    printf("[SERVER]: Received ACK: %d\n", ack_value);

    if (ack_value != syn_ack_value + 1) {
        printf("[SERVER]: Invalid ACK received. Handshake failed\n");
        exit(EXIT_FAILURE);
    }

    printf("[SERVER]: Handshake complete\n");

    return downstream;
}
