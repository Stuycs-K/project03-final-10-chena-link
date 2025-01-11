#include <fcntl.h>
#include <signal.h>
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

    this->status = SSTATUS_OPEN;
    this->max_clients = 2;
    this->current_clients = 0;
    this->id = server_id;

    this->name = NULL;

    this->connection_handler_pid = -1;
    this->connection_handler_recv_queue = net_event_queue_new();

    pipe(this->connection_handler_pipe);
    set_nonblock(this->connection_handler_pipe[PIPE_READ]);

    this->clients = malloc(sizeof(Client *) * this->max_clients);
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
        this->clients = realloc(this->clients, sizeof(Client *) * max_clients);

        for (int i = old_max_clients - 1; i < max_clients; ++i) {
            this->clients[i] = client_connection_new(i);
        }
    } else {
        for (int i = old_max_clients - 1; i >= max_clients - 1; --i) {
            free_client_connection(this->clients[i]);
        }

        this->clients = realloc(this->clients, sizeof(Client *) * max_clients);
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

void handle_client_connection(Server *this, NetEvent *handshake_event) {
    NetArgs_Handshake *handshake = handshake_event->args;

    int client_id = get_free_client_id(this);
    // handshake->client_id = client_id;

    Client *connection = this->clients[client_id];

    connection->is_free = CONNECTION_IS_USED;

    // THE HANDSHAKE FDs ARE NOT THE MAIN SERVER'S FDS! We have to use this magic to open the process's FDs as our own.
    char fd_path[64];
    snprintf(fd_path, sizeof(fd_path), "/proc/%d/fd/%d", this->connection_handler_pid, handshake->client_to_server_fd);
    connection->recv_fd = open(fd_path, O_RDONLY);

    snprintf(fd_path, sizeof(fd_path), "/proc/%d/fd/%d", this->connection_handler_pid, handshake->server_to_client_fd);
    connection->send_fd = open(fd_path, O_WRONLY);

    printf("CLIENT CONNECTED %d \n", connection->recv_fd);

    this->current_clients++;
}

void handle_client_disconnect(Server *this, int client_id) {
    disconnect_client(this->clients[client_id]);

    this->current_clients--;
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

void handle_connections(Server *this) {
    NetEventQueue *queue = this->connection_handler_recv_queue;
    empty_net_event_queue(queue);

    int connection_handler_read_fd = this->connection_handler_pipe[PIPE_READ];

    struct pollfd check;
    check.fd = connection_handler_read_fd;
    check.events = POLLIN;

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
            if (event->protocol == HANDSHAKE) {
                handle_client_connection(this, event);
            }
        }
    }

    event_buffer = NULL; // I don't know if this is necessary
    empty_net_event_queue(queue);
}

struct pollfd get_pollfd_for_client(struct pollfd *pollfds, int size, int fd) {
    for (int i = 0; i < size; ++i) {
        struct pollfd current = pollfds[i];
        if (current.fd == fd) {
            return current;
        }
    }
}

void server_recv_events(Server *this) {
    // Setup poll
    int pollcount = 0;
    struct pollfd pollfds[this->max_clients];

    FOREACH_CLIENT(this) {
        struct pollfd pollfd;
        pollfd.events = POLLIN;
        pollfd.fd = client->recv_fd;

        pollfds[pollcount++] = pollfd;
    }
    END_FOREACH_CLIENT

    int ret = poll(pollfds, this->current_clients, 0);
    if (ret <= 0) { // Nobody sent anything
        return;
    }

    FOREACH_CLIENT(this) {
        struct pollfd poll_request = get_pollfd_for_client(pollfds, this->current_clients, client->recv_fd);

        if (poll_request.revents & POLLERR || poll_request.revents & POLLHUP || poll_request.revents & POLLNVAL) {
            handle_client_disconnect(this, client_id);
            continue;
        }

        if (!(poll_request.revents & POLLIN)) { // This client hasn't done anything
            continue;
        }

        char *event_buffer = read_into_buffer(client->recv_fd);

        NetEventQueue *queue = client->recv_queue;
        recv_event_queue(queue, event_buffer);

        printf("ID: %d | Count: %d \n", client_id, queue->event_count);

        for (int i = 0; i < queue->event_count; ++i) {
            handle_core_server_net_event(this, client_id, queue->events[i]);
        }
    }
    END_FOREACH_CLIENT
}

// Call after you finish processing all NetEvents
void server_empty_recv_events(Server *this) {
    FOREACH_CLIENT(this) {
        empty_net_event_queue(client->recv_queue);
    }
    END_FOREACH_CLIENT
}

/*
    Queue a NetEvent to be sent to the client at the ID.
*/
void server_send_event_to(Server *this, int client_id, NetEvent *event) {
    insert_event(this->clients[client_id]->send_queue, event);
}

/*
    Queue a NetEvent to be sent to all currently connected clients.
*/
void server_send_event_to_all(Server *this, NetEvent *event) {
    FOREACH_CLIENT(this) {
        server_send_event_to(this, client_id, event);
    }
    END_FOREACH_CLIENT
}

/*
    Dispatches and empties each client's send queue
*/
void server_send_events(Server *this) {
    FOREACH_CLIENT(this) {
        send_event_queue(client->send_queue, client->send_fd);
        empty_net_event_queue(client->send_queue);
    }
    END_FOREACH_CLIENT
}

void server_start_connection_handler(Server *this) {
    pid_t pid = fork();
    if (pid == 0) {
        connection_handler_init(this);
    } else {
        this->connection_handler_pid = pid;
    }
}

void server_shutdown(Server *this) {
}

void server_run(Server *this) {
    server_start_connection_handler(this);

    while (1) {
        handle_connections(this);

        server_recv_events(this);
        server_empty_recv_events(this);

        server_send_events(this);
        usleep(TICK_TIME_MICROSECONDS);
    }
}
