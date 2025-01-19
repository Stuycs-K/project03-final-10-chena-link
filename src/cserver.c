#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "cserver.h"
#include "network/pipenetevents.h"
#include "shared.h"

/*
    Construct a new CServer, which initializes new GServers.

    PARAMS:
        int id : just a random number. Not important.

    RETURNS: the new CServer
*/
CServer *cserver_new(int id) {
    CServer *this = malloc(sizeof(CServer));

    GServerInfoList *info_list = nargs_gserver_info_list();
    this->server_list_event = net_event_new(GSERVER_LIST, info_list);
    this->server_list_updated = 0;

    this->gserver_count = MAX_CSERVER_GSERVERS;

    this->gserver_list = malloc(sizeof(GServer *) * this->gserver_count);
    this->server = server_new(id);
    server_set_max_clients(this->server, MAX_CSERVER_CLIENTS);

    strcpy(this->server->name, "CentralServer");
    strcpy(this->server->wkp_name, CSERVER_WKP_NAME); // WKP is "CSERVER"

    for (int gserver_id = 0; gserver_id < this->gserver_count; ++gserver_id) {
        GServer *new_gserver = gserver_new(gserver_id);
        new_gserver->status = GSS_UNRESERVED;
        this->gserver_list[gserver_id] = new_gserver;

        GServerInfo *current_gserver_info = info_list[gserver_id];

        // Initialize our copy of GServerInfo for this GServer
        current_gserver_info->id = gserver_id;
        current_gserver_info->status = GSS_UNRESERVED;
        current_gserver_info->current_clients = 0;
        current_gserver_info->max_clients = new_gserver->server->max_clients;
        strcpy(current_gserver_info->name, new_gserver->server->name);
        strcpy(current_gserver_info->wkp_name, new_gserver->server->wkp_name);
    }

    return this;
}

/*
    Responds to the ReserveGServer event. If an unreserved GServer is found,
    the CServer forks and runs the GServer.
    The CServer then sends the client that asked for a reservation the ID of the
    started GServer, which they can then join.

    PARAMS:
        CServer *this : the CServer
        int client_id : which client is asking for a reservation

    RETURNS: none
*/
void reserve_gserver(CServer *this, int client_id) {
    GServer *gserver = NULL;

    ReserveGServer *reserve_info = nargs_reserve_gserver();
    NetEvent *reserve_event = net_event_new(RESERVE_GSERVER, reserve_info);

    GServerInfo **server_info_list = this->server_list_event->args;

    // Get unreserved GServer
    for (int i = 0; i < this->gserver_count; ++i) {
        if (server_info_list[i]->status == GSS_UNRESERVED) {
            gserver = this->gserver_list[i];
            break;
        }
    }

    if (!gserver) { // Send -1 to the client to let them know they can't reserve any more servers!
        server_send_event_to(this->server, client_id, reserve_event);
        return;
    }

    pipe(gserver->cserver_pipes);

    pid_t pid = fork();
    if (pid == 0) {
        gserver_run(gserver);
    } else {
        // Tell the client which server they reserved so they can join!
        reserve_info->gserver_id = gserver->server->id;
        gserver->server->pid = pid;

        server_send_event_to(this->server, client_id, reserve_event);
    }
}

/*
    Updates the CServer's list of information about the GServers.
    Handles a GServer's NetEvent that they send when a client joins.

    PARAMS:
        CServer *this : the CServer
        GServerInfo *recv_server_info : the GServer's new, updated information

    RETURNS: none
*/
static void update_gserver_list(CServer *this, GServerInfo *recv_server_info) {
    NetEvent *server_list_event = this->server_list_event;
    GServerInfoList *server_list = server_list_event->args;

    GServerInfo *local_server_info = server_list[recv_server_info->id];

    local_server_info->status = recv_server_info->status;
    local_server_info->current_clients = recv_server_info->current_clients;
    local_server_info->max_clients = recv_server_info->max_clients;
    strcpy(local_server_info->name, recv_server_info->name);

    this->server_list_updated = 1;
}

/*
    Sends the server list to all clients.
    Called when the CServer receives an update about a GServer.

    PARAMS:
        CServer *this : the CServer

    RETURNS: none
*/
void cserver_send_server_list(CServer *this) {
    NetEvent *server_list_event = this->server_list_event;
    // GServerInfoList *server_list = server_list_event->args;

    // Attach server list event to clients' send queue to automatically send it (no mallocing a new one).
    // Only do this if they just joined OR a GServer status changed.
    FOREACH_CLIENT(this->server) {
        if (client->recently_connected || this->server_list_updated) {
            attach_event(client->send_queue, server_list_event);
        } else {
            detach_event(client->send_queue, server_list_event);
        }
    }
    END_FOREACH_CLIENT()

    this->server_list_updated = 0;
}

/*
    Sends SIGINT to the GServer with the given ID

    PARAMS:
        CServer *this : the CServer
        int gserver_id : which GServer to shut down

    RETURNS: none
*/
void kill_gserver(CServer *this, int gserver_id) {
    GServer *gserver = this->gserver_list[gserver_id];
    kill(gserver->server->pid, SIGINT);
    gserver->server->pid = -1;
    gserver->status = GSS_UNRESERVED;

    remove(gserver->server->wkp_name); // OH GOD

    // JUST IN CASE
    NetEvent *server_list_event = this->server_list_event;
    GServerInfoList *server_list = server_list_event->args;
    GServerInfo *local_server_info = server_list[gserver_id];
    local_server_info->status = GSS_UNRESERVED;
    local_server_info->current_clients = 0;

    this->server_list_updated = 1;

    close(gserver->cserver_pipes[PIPE_READ]);
    close(gserver->cserver_pipes[PIPE_WRITE]);

    gserver->cserver_pipes[PIPE_READ] = -1;
    gserver->cserver_pipes[PIPE_WRITE] = -1;
}

/*
    Calls handlers for GServer NetEvents.
    Current NetEvents: GSERVER_INFO (update_gserver_list)

    PARAMS:
        CServer *this : the CServer
        int gserver_id : which GServer
        NetEvent *event : the received event

    RETURNS: none
*/
void cserver_handle_gserver_net_event(CServer *this, int gserver_id, NetEvent *event) {
    void *args = event->args;

    switch (event->protocol) {

    case GSERVER_INFO: {
        GServerInfo *server_info = args;

        // Everybody in this GServer left. Shut it down!
        if (server_info->current_clients == 0 && server_info->status == GSS_SHUTTING_DOWN) {
            kill_gserver(this, gserver_id);
            server_info->status = GSS_UNRESERVED; // Locally set the GServerInfo to unreserved
            server_info->current_clients = 0;
        }

        printf("SERVER LIST UPDATED\n");
        update_gserver_list(this, server_info);
    }

    default:
        break;
    }
}

/*
    Polls and adds GServer NetEvents to their queues.
    Afterwards, they're processed with cserver_handle_gserver_net_event.
    This is very similar to the client polling done by the base Server.

    PARAMS:
        CServer *this : the CServer

    RETURNS: none
*/
void cserver_recv_gserver_events(CServer *this) {
    // Setup poll
    int pollcount = 0;
    struct pollfd pollfds[this->gserver_count];

    for (int i = 0; i < this->gserver_count; ++i) {
        GServer *gserver = this->gserver_list[i];

        struct pollfd pollfd;
        pollfd.events = POLLIN;
        pollfd.fd = gserver->cserver_pipes[PIPE_READ];

        pollfds[pollcount++] = pollfd;
    }

    int ret = poll(pollfds, pollcount, 0);
    if (ret <= 0) { // No GServer updates
        return;
    }

    for (int gserver_id = 0; gserver_id < this->gserver_count; ++gserver_id) {
        GServer *gserver = this->gserver_list[gserver_id];
        if (gserver->server->pid == -1) {
            continue;
        }

        struct pollfd poll_request = pollfds[gserver_id];

        if (poll_request.revents & POLLERR || poll_request.revents & POLLHUP || poll_request.revents & POLLNVAL) {
            kill_gserver(this, gserver_id);
            continue;
        }

        if (!(poll_request.revents & POLLIN)) { // GServer sent nothing
            continue;
        }

        char *event_buffer = read_into_buffer(gserver->cserver_pipes[PIPE_READ]);

        NetEventQueue *queue = gserver->cserver_recv_queue;
        recv_event_queue(queue, event_buffer);

        for (int i = 0; i < queue->event_count; ++i) {
            cserver_handle_gserver_net_event(this, gserver_id, queue->events[i]);
        }
    }

    for (int i = 0; i < this->gserver_count; ++i) {
        GServer *gserver = this->gserver_list[i];
        clear_event_queue(gserver->cserver_recv_queue);
    }
}

/*
    Calls handlers for client NetEvents.
    Current NetEvents: RESERVE_GSERVER

    PARAMS:
        CServer *this : the CServer
        int client_id : which client sent the event
        NetEvent *event : the event

    RETURNS: none
*/
void cserver_handle_net_event(CServer *this, int client_id, NetEvent *event) {
    void *args = event->args;

    switch (event->protocol) {

    case RESERVE_GSERVER: {
        ReserveGServer *nargs = args;
        reserve_gserver(this, client_id);
        break;
    }

    default:
        break;
    }
}

/*
    Runs one tick of CServer logic.
    1) Handle each client's NetEvents
    2) Receive and handle each GServer's NetEvents.
    3) Update clients on the GServers, if needed.

    PARAMS:
        CServer *this : the CServer

    RETURNS: none
*/
void cserver_loop(CServer *this) {
    Server *server = this->server;

    FOREACH_CLIENT(server) {
        NetEventQueue *queue = client->recv_queue;
        for (int i = 0; i < queue->event_count; ++i) {
            cserver_handle_net_event(this, client_id, queue->events[i]);
        }
    }
    END_FOREACH_CLIENT()

    cserver_recv_gserver_events(this);

    cserver_send_server_list(this);
}

/*
    Essentially the main function of the CServer.
    Runs the loop that loops all of the logic.

    PARAMS:
        CServer *this : the CServer

    RETURNS: none
*/
void cserver_run(CServer *this) {
    Server *server = this->server;

    server_start_connection_handler(server);

    while (1) {
        handle_connections(server);

        server_empty_recv_events(server);
        server_recv_events(server);

        cserver_loop(this);

        server_send_events(server);
        usleep(TICK_TIME_MICROSECONDS);
    }
}
