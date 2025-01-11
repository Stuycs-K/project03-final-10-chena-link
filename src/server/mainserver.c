#include <stdio.h>
#include <unistd.h>

#include "../network/pipehandshake.h"
#include "../network/pipenetevents.h"
#include "../shared.h"
#include "../util/file.h"
#include "connectionhandler.h"
#include "mainserver.h"

Server *server_new(int server_id) {
    Server *this = malloc(sizeof(Server));

    this->max_clients = 2;
    this->current_clients = 0;
    this->id = server_id;

    this->name = NULL;
    this->client_poll_list = poll_list_new(128); // 128 for no reason
    this->connection_handler_recv_queue = net_event_queue_new();

    pipe(this->connection_handler_pipe);
    set_nonblock(this->connection_handler_pipe[PIPE_READ]);

    this->clients = malloc(sizeof(ClientConnection *) * this->max_clients);
    for (int i = 0; i < this->max_clients; ++i) {
        this->clients[i] = client_connection_new(i);
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
        this->clients = realloc(this->clients, sizeof(ClientConnection *) * max_clients);

        for (int i = old_max_clients - 1; i < max_clients; ++i) {
            this->clients[i] = client_connection_new(i);
        }
    } else {
        for (int i = old_max_clients - 1; i >= max_clients - 1; --i) {
            free_client_connection(this->clients[i]);
        }

        this->clients = realloc(this->clients, sizeof(ClientConnection *) * max_clients);
    }
}

// -1 indicates the server is full
int get_free_client_id(Server *this) {
    for (int i = 0; i < this->max_clients; ++i) {
        if (this->clients[i]->is_free) {
            return i;
        }
    }

    return -1;
}

ClientConnection *init_client_connection(Server *this, NetEvent *handshake_event) {
    NetArgs_InitialHandshake *handshake = handshake_event->args;

    int client_id = get_free_client_id(this);
    // handshake->client_id = client_id;

    ClientConnection *connection = this->clients[client_id];

    connection->is_free = CONNECTION_IS_USED;

    connection->recv_fd = handshake->client_to_server_fd;
    connection->send_fd = handshake->server_to_client_fd;

    // Register for polling.
    struct pollfd *request = insert_pollfd(this->client_poll_list, client_id, connection->recv_fd);
    request->events = POLLIN; // Only poll for new data to read

    this->current_clients++;
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

void handle_new_connections(Server *this) {
    int connection_handler_read_fd = this->connection_handler_pipe[PIPE_READ];

    struct pollfd check;
    check.fd = connection_handler_read_fd;
    check.events = POLLIN;

    NetEventQueue *queue = this->connection_handler_recv_queue;

    empty_net_event_queue(queue);

    int ret = poll(&check, 1, 0);

    if (ret <= 0) {
        return;
    }

    char *event_buffer;
    while (event_buffer = read_into_buffer(connection_handler_read_fd)) {
        recv_event_queue(queue, event_buffer);

        for (int i = 0; i < queue->event_count; ++i) {
            NetEvent *event = queue->events[i];
            void *args = event->args;

            // Mark client as connected
            if (event->protocol == INITIAL_HANDSHAKE) {
                init_client_connection(this, event);
            }
        }
    }

    event_buffer = NULL; // I don't know if this is necessary
    empty_net_event_queue(queue);
}

void server_recv_events(Server *this) {
    PollList *polls = this->client_poll_list;

    int ret = poll(*polls->requests, polls->count, 0);
    if (ret <= 0) { // Nobody sent anything
        return;
    }

    for (int client_id = 0; client_id < this->max_clients; ++client_id) {
        ClientConnection *client = this->clients[client_id];
        if (client->is_free) {
            continue;
        }

        struct pollfd *poll_request = polls->requests[client_id];

        if (poll_request->revents & POLLERR) {
            disconnect_client(client);
            // Client disconnect.
        }
    }
}

/*
    Dispatches and empties each client's send queue
*/
void server_send_events(Server *this) {
    for (int i = 0; i < this->max_clients; ++i) {
        ClientConnection *client = this->clients[i];
        if (client->is_free) {
            continue;
        }

        send_event_queue(client->send_queue, client->send_fd);
        empty_net_event_queue(client->send_queue);
    }
}

void server_run(Server *this) {
    pid_t pid = fork();
    if (pid == 0) {
        connection_handler_init(this);
    }

    // int recv_fd = this->subserver_pipe[PIPE_READ];
    // set_nonblock(recv_fd);

    struct pollfd *polls;

    while (1) {
        handle_new_connections(this);

        // 2) Read all NetEvents

        int client_id;
        ssize_t bytes_read;

        /*
        while (1) {
            bytes_read = read(STDIN_FILENO, &client_id, sizeof(client_id));
            // Done reading all messages
            if (bytes_read <= 0) {
                break;
            }

            char *event_buffer = read_into_buffer(STDIN_FILENO);
            recv_event_queue(subserver_recv_queue, event_buffer);

            printf("ID: %d | Count: %d \n", client_id, subserver_recv_queue->event_count);

            // ALL of these events come from client_id
            for (int i = 0; i < subserver_recv_queue->event_count; ++i) {
                handle_core_server_net_event(this, client_id, subserver_recv_queue->events[i]);
            }
        }
        */

        usleep(TICK_TIME_MICROSECONDS);
    }
}
