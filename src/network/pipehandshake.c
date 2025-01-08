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
#include "pipenet.h"
#include "pipenetevents.h"

#define HANDSHAKE_DEBUG

int server_setup(char *client_to_server_fifo) {
    remove(client_to_server_fifo);

    int mkfifo_ret = mkfifo(client_to_server_fifo, 0644);

    printf("[SERVER]: Waiting for connection...\n");

    int from_client = open(client_to_server_fifo, O_RDONLY, 0);

    printf("[SERVER]: Client connected\n");

    remove(client_to_server_fifo);

    return from_client;
}

NetEvent *create_handshake_event() {
    NetArgs_InitialHandshake *handshake_args = nargs_initial_handshake();
    NetEvent *handshake_event = net_event_new(INITIAL_HANDSHAKE, handshake_args);
}

void free_handshake_event(NetEvent *handshake_event) {
    NetArgs_InitialHandshake *handshake_args = handshake_event->args;
    free(handshake_args->to_client_pipe_name);

    free(handshake_event->args);
    free(handshake_event);
}

void server_abort_handshake(int send_fd, NetEvent *handshake_event, HandshakeErrCode errcode) {
    NetArgs_InitialHandshake *handshake_args = handshake_event->args;
    handshake_args->errcode = errcode;

    send_event_immediate(handshake_event, send_fd);
}

int server_get_send_fd(int recv_fd, NetEvent *handshake_event) {
    NetArgs_InitialHandshake *handshake_args = handshake_event->args;

    printf("[SERVER]: Waiting for SYN...\n");

    recv_event_immediate(recv_fd, handshake_event);

    char *to_client_pipe_name = handshake_args->to_client_pipe_name;

    int send_fd = open(to_client_pipe_name, O_WRONLY, 0);

    return send_fd;
}

int server_complete_handshake(int recv_fd, int send_fd, NetEvent *handshake_event) {
    // SYN-ACK
    int syn_ack_value = rand();
    NetArgs_InitialHandshake *handshake_args = handshake_event->args;
    handshake_args->syn_ack = syn_ack_value;

    send_event_immediate(handshake_event, send_fd);

    // ACK
    recv_event_immediate(recv_fd, handshake_event);

    // Inform the client that they failed the ACK.
    if (handshake_args->ack != handshake_args->syn_ack + 1) {
        printf("[SERVER]: Invalid ACK received. Handshake failed\n");

        server_abort_handshake(send_fd, handshake_event, HEC_INVALID_ACK);

        return -1;
    }

    // Send the event back so the client knows their ACK was correct.
    send_event_immediate(handshake_event, send_fd);
    return 1;
}

int client_setup(char *client_to_server_fifo, NetEvent *handshake_event) {
    pid_t client_pid = getpid();
    char pid_string[HANDSHAKE_BUFFER_SIZE];
    snprintf(pid_string, sizeof(pid_string), "%d", client_pid);
    remove(pid_string); // Just in case

    // Copy over SYN
    NetArgs_InitialHandshake *handshake_args = handshake_event->args;
    strcpy(handshake_args->to_client_pipe_name, pid_string);

    int mkfifo_ret = mkfifo(pid_string, 0644);

    int to_server = open(client_to_server_fifo, O_WRONLY, 0);
    return to_server;
}

HandshakeErrCode client_recv_handshake_event(int from_server, NetEvent *handshake_event) {
    NetArgs_InitialHandshake *handshake_args = handshake_event->args;

    recv_event_immediate(from_server, handshake_event);

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

int client_handshake(int send_fd, NetEvent *handshake_event) {
    NetArgs_InitialHandshake *handshake_args = handshake_event->args;

    send_event_immediate(handshake_event, send_fd);

    int from_server = open(handshake_args->to_client_pipe_name, O_RDONLY, 0);
    remove(handshake_args->to_client_pipe_name);

    if (client_recv_handshake_event(from_server, handshake_event) != HEC_SUCCESS) {
        return -1;
    }

    handshake_args->ack = handshake_args->syn_ack + 1;
    send_event_immediate(handshake_event, send_fd);

    if (client_recv_handshake_event(from_server, handshake_event) != HEC_SUCCESS) {
        return -1;
    }

    return from_server;
}
