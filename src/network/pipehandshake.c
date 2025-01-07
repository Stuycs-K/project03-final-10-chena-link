#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "../debug/debug.h"
#include "pipehandshake.h"

#define HANDSHAKE_DEBUG

int server_setup(char *client_to_server_fifo) {
    remove(client_to_server_fifo);

    int mkfifo_ret = mkfifo(client_to_server_fifo, 0644);

    printf("[SERVER]: Waiting for connection...\n");

    int from_client = open(client_to_server_fifo, O_RDONLY, 0);
    fatal_assert(from_client, "Open client PP fail\n");

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

    int syn_ack_value = rand();
    ssize_t bytes = write(downstream, &syn_ack_value, sizeof(syn_ack_value));

    printf("[SERVER]: Sent SYN-ACK: %d\n", syn_ack_value);
    printf("[SERVER]: Waiting for ACK...\n");

    int ack_value;
    bytes = read(from_client, &ack_value, sizeof(ack_value));

    printf("[SERVER]: Received ACK: %d\n", ack_value);

    if (ack_value != syn_ack_value + 1) {
        printf("[SERVER]: Invalid ACK received. Handshake failed\n");
        return -1;
    }

    printf("[SERVER]: Handshake complete\n");

    return downstream;
}

void client_handshake(char *client_to_server_fifo, int *fd_pair) {
    // Create PID string, used as name of PP and value of SYN
    pid_t client_pid = getpid();
    char pid_string[HANDSHAKE_BUFFER_SIZE];
    snprintf(pid_string, sizeof(pid_string), "%d", client_pid);

    remove(pid_string); // Just in case
    int mkfifo_ret = mkfifo(pid_string, 0644);

    printf("[CLIENT]: Created PP: %s\n", pid_string);

    int to_server = open(client_to_server_fifo, O_WRONLY, 0);
    fd_pair[0] = to_server;

    // Write SYN
    ssize_t bytes = write(to_server, pid_string, sizeof(pid_string));

    printf("[CLIENT]: Sent SYN: %d\n", client_pid);
    printf("[CLIENT]: Waiting for SYN-ACK...\n");

    int from_server = open(pid_string, O_RDONLY, 0);
    fd_pair[1] = from_server;

    remove(pid_string);

    int syn_ack_value;
    bytes = read(from_server, &syn_ack_value, sizeof(syn_ack_value));

    printf("[CLIENT]: Received SYN-ACK: %d\n", syn_ack_value);

    int ack_value = syn_ack_value + 1;
    bytes = write(to_server, &ack_value, sizeof(ack_value));

    printf("[CLIENT]: Sent ACK: %d\n", ack_value);
    printf("[CLIENT]: Handshake complete\n");
}
