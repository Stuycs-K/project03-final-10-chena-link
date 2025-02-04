#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "pipehandshake.h"
#include "pipenet.h"
#include "pipenetevents.h"

#define HANDSHAKE_DEBUG

NetEvent *create_handshake_event() {
    Handshake *nargs = nargs_handshake();
    NetEvent *handshake_event = net_event_new(HANDSHAKE, nargs);
    return handshake_event;
}

void free_handshake_event(NetEvent *handshake_event) {
    Handshake *handshake_args = handshake_event->args;
    free(handshake_args->to_client_pipe_name);

    free(handshake_event->args);
    free(handshake_event);
}

NetEvent *server_setup(char *client_to_server_fifo) {
    NetEvent *handshake_event = create_handshake_event();
    Handshake *handshake = handshake_event->args;

    remove(client_to_server_fifo);

    int mkfifo_ret = mkfifo(client_to_server_fifo, 0666);

    int from_client = open(client_to_server_fifo, O_RDONLY, 0);

    printf("[SERVER]: Client connected\n");

    remove(client_to_server_fifo);

    handshake->client_to_server_fd = from_client;

    return handshake_event;
}

void server_abort_handshake(NetEvent *handshake_event, HandshakeErrCode errcode) {
    Handshake *handshake_args = handshake_event->args;

    handshake_args->errcode = errcode;

    send_event_immediate(handshake_event, handshake_args->server_to_client_fd);
}

void server_get_send_fd(NetEvent *handshake_event) {
    Handshake *handshake_args = handshake_event->args;

    printf("[SERVER]: Waiting for SYN...\n");

    recv_event_immediate(handshake_args->client_to_server_fd, handshake_event);

    int send_fd = open(handshake_args->to_client_pipe_name, O_WRONLY, 0);
    handshake_args->server_to_client_fd = send_fd;
}

int server_complete_handshake(NetEvent *handshake_event) {
    // SYN-ACK
    int syn_ack_value = rand();
    Handshake *handshake_args = handshake_event->args;
    handshake_args->syn_ack = syn_ack_value;

    int send_fd = handshake_args->server_to_client_fd;
    int recv_fd = handshake_args->client_to_server_fd;

    printf("[SERVER]: Sending SYN-ACK\n");

    // SYN-ACK
    send_event_immediate(handshake_event, send_fd);

    printf("[SERVER]: Waiting for ACK...\n");

    // ACK
    recv_event_immediate(handshake_args->client_to_server_fd, handshake_event);

    // Inform the client that they failed the ACK.
    if (handshake_args->ack != handshake_args->syn_ack + 1) {
        printf("[SERVER]: Invalid ACK received. Handshake failed\n");

        server_abort_handshake(handshake_event, HEC_INVALID_ACK);

        return -1;
    }

    printf("[SERVER]: Handshake complete\n");
    return 1;
}

HandshakeErrCode client_recv_handshake_event(NetEvent *handshake_event) {
    Handshake *handshake_args = handshake_event->args;

    recv_event_immediate(handshake_args->server_to_client_fd, handshake_event);

    if (handshake_args->errcode != -1) {

        switch (handshake_args->errcode) {

        case HEC_INVALID_ACK:
            printf("Failed ACK\n");
            break;

        case HEC_SERVER_IS_FULL:
            printf("Server is full\n");
            break;

        default:
            break;
        }

        return handshake_args->errcode;
    }

    return HEC_SUCCESS;
}

void client_setup(char *client_to_server_fifo, NetEvent *handshake_event) {
    pid_t client_pid = getpid();
    char pid_string[HANDSHAKE_BUFFER_SIZE];
    snprintf(pid_string, sizeof(pid_string), "%d", client_pid);
    remove(pid_string); // Just in case

    // Copy over SYN
    Handshake *handshake_args = handshake_event->args;
    strcpy(handshake_args->to_client_pipe_name, pid_string);

    int mkfifo_ret = mkfifo(pid_string, 0666);

    int to_server = open(client_to_server_fifo, O_WRONLY, 0);
    handshake_args->client_to_server_fd = to_server;
}

int client_handshake(NetEvent *handshake_event) {
    Handshake *handshake_args = handshake_event->args;
    int send_fd = handshake_args->client_to_server_fd;

    send_event_immediate(handshake_event, send_fd);

    int from_server = open(handshake_args->to_client_pipe_name, O_RDONLY, 0);

    remove(handshake_args->to_client_pipe_name);

    handshake_args->server_to_client_fd = from_server;

    if (client_recv_handshake_event(handshake_event) != HEC_SUCCESS) {
        return -1;
    }

    handshake_args->ack = handshake_args->syn_ack + 1;
    send_event_immediate(handshake_event, send_fd);

    return 1;
}
