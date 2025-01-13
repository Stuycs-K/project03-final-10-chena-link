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

/*
    Constructs a new Server.

    Allocates clients and sets up connection handler pipe.

    PARAMS:
        int server_id : an identifier for the server

    RETURNS: the new Server
*/
Server *server_new(int server_id) {
    Server *this = malloc(sizeof(Server));

    this->status = SSTATUS_OPEN;

    this->max_clients = 2;
    this->current_clients = 0;
    this->id = server_id;

    strcpy(this->name, "Server"); // Set default name
    strcpy(this->wkp_name, "TEMP");

    this->connection_handler_pid = -1;
    this->connection_handler_recv_queue = net_event_queue_new();

    pipe(this->connection_handler_pipe);
    set_nonblock(this->connection_handler_pipe[PIPE_READ]);

    this->client_info_list = NULL;
    this->clients = malloc(sizeof(Client *) * this->max_clients);
    for (int i = 0; i < this->max_clients; ++i) {
        this->clients[i] = client_connection_new(i);
    }

    return this;
}

/*
    Changes the max client count.
    Allocates new Clients if we're increasing max clients.
    Frees Clients if we're decreasing max clients.

    PARAMS:
        Server *this : the Server
        int max_clients : the new value for max clients

    RETURNS: none
*/
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

    this->max_clients = max_clients;
}

/*
    Gets a Client object that isn't currently handling communication.

    PARAMS:
        Server *this : the Server

    RETURNS: a free client ID. -1 if none can be found.
*/
int get_free_client_id(Server *this) {
    for (int i = 0; i < this->max_clients; ++i) {
        if (this->clients[i]->is_free) {
            return i;
        }
    }

    return -1;
}

void send_client_list(Server *this, int client_id) {
    if (!this->client_info_changed) {
        return;
    }

    ClientList *event_args = nargs_client_list();
    event_args->local_client_id = client_id;                          // So the client knows who they are
    event_args->info_list = copy_client_list(this->client_info_list); // Copy the linked list

    NetEvent *event = net_event_new(CLIENT_LIST, event_args);

    server_send_event_to(this, client_id, event);
}

/*
    Handles a new client connecting to the server.
    Gets a Client object to store connection information (FDs mainly)
    Marks the Client object as recently connected, then updates the client info list.

    PARAMS:
        Server *this : the Server
        NetEvent *handshake_event : NetEvent containing the client's handshake performed with the connection handler

    RETURNS: none
*/
void handle_client_connection(Server *this, NetEvent *handshake_event) {
    Handshake *handshake = handshake_event->args;

    int client_id = get_free_client_id(this);
    Client *client = this->clients[client_id];

    client->is_free = CONNECTION_IS_USED;

    // THE FDs IN THE HANDSHAKE ARE NOT THE MAIN SERVER'S FDS! THIS IS A REALLY TERRIBLE SOLUTION
    // The connection handler (separate process) opens the pipes, so the FDs aren't shared with the main server.
    // We have to use this magic to open the process's FDs as our own.
    char fd_path[64];
    snprintf(fd_path, sizeof(fd_path), "/proc/%d/fd/%d", this->connection_handler_pid, handshake->client_to_server_fd);
    client->recv_fd = open(fd_path, O_RDONLY);

    snprintf(fd_path, sizeof(fd_path), "/proc/%d/fd/%d", this->connection_handler_pid, handshake->server_to_client_fd);
    client->send_fd = open(fd_path, O_WRONLY);

    client->recently_connected = 1;

    printf("CLIENT CONNECTED %d \n", client->recv_fd);

    this->current_clients++;

    // Update client info list
    this->client_info_changed = 1;
    this->client_info_list = insert_client_list(this->client_info_list, client_id);
    strcpy(this->client_info_list->name, handshake->client_name);
}

/*
    Handles a client disconnecting from the server.
    Updates the client info list.

    PARAMS:
        Server *this : the Server
        int client_id : the ID of the client disconnecting

    RETURNS: none
*/
void handle_client_disconnect(Server *this, int client_id) {
    disconnect_client(this->clients[client_id]);

    // Update client info list
    this->client_info_list = remove_client_list_by_id(this->client_info_list, client_id);

    // This gets called during the receive phase.
    // By this point, if any clients joined, we've already sent the client list to each client's queue.
    // That means, of course, we have to go into each client's send queue and update the event. Wow.
    // Otherwise, if no one else joined this tick, we have to send to client list to everyone.
    if (this->client_info_changed) {
        FOREACH_CLIENT(this) {
            NetEventQueue *send_queue = client->send_queue;
            for (int i = 0; i < send_queue->event_count; ++i) {
                NetEvent *event = send_queue->events[i];
                if (event->protocol == CLIENT_LIST) {
                    ClientList *nargs = event->args;
                    nargs->info_list = remove_client_list_by_id(nargs->info_list, client_id);
                }
            }
        }
        END_FOREACH_CLIENT()
    } else {
        this->client_info_changed = 1;

        FOREACH_CLIENT(this) {
            send_client_list(this, client_id);
        }
        END_FOREACH_CLIENT()
    }

    this->clients[client_id]->recently_disconnected = 1;
    this->current_clients--;
}

/*
    Handles a client disconnecting from the server.
    Updates the client info list.

    PARAMS:
        Server *this : the Server
        int client_id : the ID of the client disconnecting

    RETURNS: none
*/
void handle_core_server_net_event(Server *this, int client_id, NetEvent *event) {
    void *args = event->args;

    // The cases are wrapped in braces so we can keep using "nargs"
    switch (event->protocol) {

    default:
        break;
    }
}

struct pollfd get_pollfd_for_client(struct pollfd *pollfds, int size, int fd) {
    for (int i = 0; i < size; ++i) {
        struct pollfd current = pollfds[i];
        if (current.fd == fd) {
            return current;
        }
    }
}

/*
    Polls the connection handler to accept new clients.
    Then updates the client info list.

    PARAMS:
        Server *this : the Server
        int client_id : the ID of the client disconnecting

    RETURNS: none
*/
void handle_connections(Server *this) {
    this->client_info_changed = 0; // Reset flag

    FOREACH_CLIENT(this) {
        if (client->recently_connected) {
            client->recently_connected = 0;
        }

        if (client->recently_disconnected) {
            client->recently_disconnected = 0;
        }
    }
    END_FOREACH_CLIENT()

    NetEventQueue *queue = this->connection_handler_recv_queue;
    clear_event_queue(queue);

    int connection_handler_read_fd = this->connection_handler_pipe[PIPE_READ];

    struct pollfd check;
    check.fd = connection_handler_read_fd;
    check.events = POLLIN;

    int ret = poll(&check, 1, 0);
    if (ret <= 0) { // Nobody connected to the connection handler
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
    clear_event_queue(queue);

    // The first event sent to all clients is the client list.
    // Newly joined clients MUST set their client_id as soon as possible.
    FOREACH_CLIENT(this) {
        send_client_list(this, client_id);
    }
    END_FOREACH_CLIENT()
}

/*
    Polls each connected client's client-to-server FD.
    If the poll returns an error event, the client is disconnected.
    If there's nothing to read from, the client is skipped.
    Otherwise, the client's FD is read into the server's receive queue for that client.

    The client's NetEvents henceforth can be accessible and manipulated in this server tick with another FOREACH_CLIENT loop.

    The base server handles some base NetEvents here, such as client name changes.

    PARAMS:
        Server *this : the Server

    RETURNS: void
*/
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
    END_FOREACH_CLIENT()

    int ret = poll(pollfds, this->current_clients, 0);
    if (ret <= 0) { // Nobody sent anything
        return;
    }

    FOREACH_CLIENT(this) {
        struct pollfd poll_request = get_pollfd_for_client(pollfds, this->current_clients, client->recv_fd);

        if (poll_request.revents & POLLERR || poll_request.revents & POLLHUP || poll_request.revents & POLLNVAL) {
            printf("CLIENT DISCONNECT\n");
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
    END_FOREACH_CLIENT()
}

/*
    Clears all clients' receive queues.
    Call after finishing processing all NetEvents

    PARAMS:
        Server *this : the Server

    RETURNS: none
*/
void server_empty_recv_events(Server *this) {
    FOREACH_CLIENT(this) {
        clear_event_queue(client->recv_queue);
    }
    END_FOREACH_CLIENT()
}

/*
    Queue a NetEvent to be sent to the client at the ID.

    PARAMS:
        Server *this : the Server
        int client_id : which client
        NetEvent *event : the NetEvent to send to the client

    RETURNS: none
*/
void server_send_event_to(Server *this, int client_id, NetEvent *event) {
    insert_event(this->clients[client_id]->send_queue, event);
}

/*
    Queue a NetEvent to be sent to all currently connected clients.

    PARAMS:
        Server *this : the Server
        NetEvent *event : the NetEvent to send to all clients

    RETURNS: none
*/
void server_send_event_to_all(Server *this, NetEvent *event) {
    FOREACH_CLIENT(this) {
        server_send_event_to(this, client_id, event);
    }
    END_FOREACH_CLIENT()
}

/*
    Sends and empties each client's send queue

    PARAMS:
        Server *this : the Server

    RETURNS: none
*/
void server_send_events(Server *this) {
    FOREACH_CLIENT(this) {
        send_event_queue(client->send_queue, client->send_fd);
        clear_event_queue(client->send_queue);
    }
    END_FOREACH_CLIENT()
}

/*
    Forks the connection handler. The Server is now able to accept connections.

    PARAMS:
        Server *this : the Server

    RETURNS: none
*/
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

/*
    UNUSED
*/
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
