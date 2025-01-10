#include <stdio.h>
#include <unistd.h>

#include "../network/pipehandshake.h"
#include "../network/pipenetevents.h"
#include "../shared.h"
#include "../util/file.h"
#include "mainserver.h"

Server *server_new(int server_id) {
    Server *this = malloc(sizeof(Server));

    this->max_clients = 2;
    this->current_clients = 0;
    this->id = server_id;

    this->name = NULL;

    pipe(this->subserver_pipe);

    // Populate with inactive subservers
    this->subservers = malloc(sizeof(Subserver *) * this->max_clients);

    for (int i = 0; i < this->max_clients; ++i) {
        this->subservers[i] = subserver_new(i);

        // Set the subserver's pipedes
        memcpy(this->subservers[i]->main_pipe, this->subserver_pipe, sizeof(this->subserver_pipe));
    }

    return this;
}

void server_set_max_clients(Server *this, int max_clients) {
    int old_max_clients = this->max_clients;

    // Don't do anything if we're not changing anything
    if (old_max_clients == max_clients) {
        return;
    }

    // Can't make server accomodate less players than it already has
    if (this->current_clients > max_clients) {
        return;
    }

    int is_expanding = max_clients > old_max_clients;

    if (is_expanding) {
        this->subservers = realloc(this->subservers, sizeof(Subserver *) * max_clients);

        for (int i = old_max_clients - 1; i < max_clients; ++i) {
            this->subservers[i] = subserver_new(i);
        }
    } else {
        for (int i = old_max_clients - 1; i >= max_clients - 1; --i) {
            free(this->subservers[i]);
        }

        this->subservers = realloc(this->subservers, sizeof(Subserver *) * max_clients);
    }
}

// -1 indicates the server is full
int get_free_client_id(Server *this) {
    for (int i = 0; i < this->max_clients; ++i) {
        if (subserver_is_inactive(this->subservers[i])) {
            return i;
        }
    }

    return -1;
}

Subserver *setup_subserver_for_connection(Server *this, NetEvent *handshake_event) {
    NetArgs_InitialHandshake *handshake = handshake_event->args;

    int client_id = get_free_client_id(this);
    handshake->client_id = client_id;

    Subserver *chosen_subserver = this->subservers[client_id];

    chosen_subserver->recv_fd = handshake->client_to_server_fd;
    chosen_subserver->send_fd = handshake->server_to_client_fd;
    chosen_subserver->client_id = client_id;
    chosen_subserver->handshake_event = handshake_event;

    this->current_clients++;

    return chosen_subserver;
}

void accept_connection(Server *this) {
    NetEvent *handshake_event = server_setup("TEMP");
    if (handshake_event == NULL) { // No clients attempting to connect on this tick.
        return;
    }

    server_get_send_fd(handshake_event);

    // A client connected, but the server is full!
    if (this->current_clients >= this->max_clients) {
        server_abort_handshake(handshake_event, HEC_SERVER_IS_FULL);
        free_handshake_event(handshake_event);

        printf("server is full!\n");
        return;
    }

    Subserver *subserver = setup_subserver_for_connection(this, handshake_event);

    pid_t pid = fork();

    if (pid == 0) {
        subserver_run(subserver);
    } else {
        this->subservers[subserver->client_id]->pid = pid;
        free_handshake_event(handshake_event);
    }
}

void handle_core_server_net_event(Server *this, int client_id, NetEvent *event) {
    void *args = event->args;

    // The cases are wrapped in braces so we can keep using "nargs"
    switch (event->protocol) {

    case PERIODIC_HANDSHAKE: {
        NetArgs_PeriodicHandshake *nargs = args;
        printf("n: %d\n", nargs->id);
        break;
    }
    case CLIENT_CONNECT: {
        NetArgs_ClientConnect *nargs = args;
        printf("CLIENT CONNECTED %d\n", client_id);
        break;
    }

    default:
        break;
    }
}

void server_run(Server *this) {
    int recv_fd = this->subserver_pipe[PIPE_READ];
    set_nonblock(recv_fd);

    NetEventQueue *subserver_recv_queue = net_event_queue_new();
    NetEventQueue *send_queue = net_event_queue_new();

    while (1) {
        // 1) Try to connect clients
        accept_connection(this);

        // 2) Read all NetEvents
        empty_net_event_queue(subserver_recv_queue);

        int client_id;
        ssize_t bytes_read;

        while (1) {
            bytes_read = read(recv_fd, &client_id, sizeof(client_id));
            // Done reading all messages
            if (bytes_read <= 0) {
                break;
            }

            char *event_buffer = read_into_buffer(recv_fd);
            recv_event_queue(subserver_recv_queue, event_buffer);

            printf("ID: %d | Count: %d \n", client_id, subserver_recv_queue->event_count);

            // ALL of these events come from client_id
            for (int i = 0; i < subserver_recv_queue->event_count; ++i) {
                handle_core_server_net_event(this, client_id, subserver_recv_queue->events[i]);
            }
        }

        usleep(TICK_TIME_MICROSECONDS);
    }
}