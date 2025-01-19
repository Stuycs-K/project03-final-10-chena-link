#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "../network/pipehandshake.h"
#include "../network/pipenet.h"
#include "../shared.h"
#include "connectionhandler.h"

char wkp_name[GSERVER_WKP_NAME_LEN];
int previous_client_to_server_fd = -1;
int previous_server_to_client_fd = -1;

void cleanup_old_fds() {
    if (previous_client_to_server_fd > -1) {
        close(previous_client_to_server_fd);
        previous_client_to_server_fd = -1;
    }

    if (previous_server_to_client_fd > -1) {
        close(previous_server_to_client_fd);
        previous_server_to_client_fd = -1;
    }
}

/*
    Signal handler that removes the WKP.
*/
static void handle_sigint(int signo) {
    if (signo != SIGINT) {
        return;
    }

    cleanup_old_fds();
    remove(wkp_name);

    exit(EXIT_SUCCESS);
}

/*
    Listens to WKP. When a client opens it, they complete the handshake.

    If they send the correct ACK, send the handshake struct back to the host server
    to inform them that the client connected and which file descriptors to use.
*/
void connection_handler_init(Server *this) {
    signal(SIGINT, handle_sigint);

    strcpy(wkp_name, this->wkp_name);

    NetEventQueue *send_host_queue = net_event_queue_new();

    int send_to_host_server_fd = this->connection_handler_pipe[PIPE_WRITE];

    while (1) {
        NetEvent *handshake_event = server_setup(wkp_name);
        Handshake *handshake = handshake_event->args;
        printf("i have initialized a connection for server %s\n", this->name);

        server_get_send_fd(handshake_event);

        pid_t pid = fork();

        if (pid == 0) {
            int status = server_complete_handshake(handshake_event);

            if (status == -1) {
                exit(EXIT_FAILURE);
            }

            handshake->are_fds_finalized = 1;
            send_event_immediate(handshake_event, send_to_host_server_fd);

            exit(EXIT_SUCCESS);
        } else {
            cleanup_old_fds();
            previous_client_to_server_fd = handshake->client_to_server_fd;
            previous_server_to_client_fd = handshake->server_to_client_fd;

            free_handshake_event(handshake_event);
        }
    }
}
