#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "game.h"
#include "gserver.h"
#include "network/pipehandshake.h"
#include "network/pipenet.h"
#include "network/pipenetevents.h"
#include "shared.h"

pid_t connection_handler_pid; // For our signal handler. This is, once again, a stupid solution to a stupid problem.

/*
    Updates the networked interface of the GServer to transmit to the CServer.
    Called whenever a player connects or disconnects.
    The only fields that will change are the server's status, current clients, maximum clients, and visible name

    PARAMS:
        GServer *this : the GServer

    RETURNS: none
*/
void update_gserver_info(GServer *this) {
    GServerInfo *server_info = this->info_event->args;

    // These are the only fields that will ever change
    server_info->current_clients = this->server->current_clients;
    server_info->max_clients = this->server->max_clients;
    server_info->status = this->status;
    server_info->host_id = this->host_client_id;
    strcpy(server_info->name, this->server->name);
}

/*
    If any clients recently joined or left, update our GServerInfo and send it
    to the CServer so they can update their information about us and inform
    all clients.

    PARAMS:
        GServer *this : the GServer

    RETURNS: none
*/
void check_update_gserver_info(GServer *this) {
    Server *server = this->server;

    int did_client_list_change = 0;

    if (server->current_clients >= 1 && this->status == GSS_RESERVED) {
        this->status = GSS_WAITING_FOR_PLAYERS;
    }

    // No players are connected right now, and we've started the server. Time to call it quits. The CServer will shut us down with SIGINT.
    if (server->current_clients == 0 && this->status != GSS_UNRESERVED && this->status != GSS_RESERVED) {
        this->status = GSS_SHUTTING_DOWN;
    }

    FOREACH_CLIENT(server) {
        if (client->recently_connected || client->recently_disconnected) {
            did_client_list_change = 1;
            update_gserver_info(this);

            break;
        }
    }
    END_FOREACH_CLIENT()

    if (did_client_list_change || this->info_changed) { // Someone joined or we marked server info as changed, so send to the CServer
        attach_event(this->cserver_send_queue, this->info_event);
    } else {
        detach_event(this->cserver_send_queue, this->info_event);
    }

    this->info_changed = 0;
}

/*
    Send any events to the CServer. This will usually just be the GServerInfo.

    PARAMS:
        GServer *this : the GServer

    RETURNS: none
*/
void send_to_cserver(GServer *this) {
    send_event_queue(this->cserver_send_queue, this->cserver_pipes[PIPE_WRITE]);
    clear_event_queue(this->cserver_send_queue);
}

/*
    Creates a new GServer.
    The default name used is "GameServer" + id, and the WKP is "G" + id.

    PARAMS:
        int id : the GServer's ID. Given by the CServer.

    RETURNS: the new GServer
*/
GServer *gserver_new(int id) {
    GServer *this = malloc(sizeof(GServer));

    GServerInfo *server_info = nargs_gserver_info();
    this->info_event = net_event_new(GSERVER_INFO, server_info);
    this->info_changed = 0;

    this->cserver_send_queue = net_event_queue_new();
    this->cserver_recv_queue = net_event_queue_new();

    this->status = GSS_UNRESERVED;

    this->host_client_id = -1;

    this->server = server_new(id);
    this->server->max_clients = DEFAULT_GSERVER_MAX_CLIENTS;

    char gserver_name_buffer[MAX_GSERVER_NAME_CHARACTERS];
    snprintf(gserver_name_buffer, sizeof(gserver_name_buffer), "GameServer%d", id);
    strcpy(this->server->name, gserver_name_buffer);

    char gserver_wkp_name_buffer[GSERVER_WKP_NAME_LEN];
    snprintf(gserver_wkp_name_buffer, sizeof(gserver_wkp_name_buffer), "G%d", id);
    strcpy(this->server->wkp_name, gserver_wkp_name_buffer);

    // Update GServerInfo
    server_info->id = id;
    strcpy(server_info->wkp_name, gserver_wkp_name_buffer);

    for (int i = 0; i < 8; i++) {
        this->decks[i] = -1;
    }
    for (int a = 0; a < 4; a++) {
        this->all_clients[a] = -1;
    }
    this->firstUNO = -1;

    update_gserver_info(this);

    return this;
}

void send_gserver_config_to_host(GServer *this) {
    GServerConfig *config = nargs_gserver_config();
    config->max_clients = this->server->max_clients;
    strcpy(config->name, this->server->name);
    config->start_game = 0;

    NetEvent *event = net_event_new(GSERVER_CONFIG, config);

    server_send_event_to(this->server, this->host_client_id, event);
}

/*

*/
void recv_gserver_config(GServer *this, int client_id, NetEvent *event) {
    GServerConfig *config = event->args;
    GServerInfo *current = this->info_event->args;

    if (config->max_clients != current->max_clients) {
        int actual_client_count = config->max_clients;

        // Clamp between 2 and 4
        if (actual_client_count < this->server->current_clients) {
            actual_client_count = this->server->current_clients;
        } else if (actual_client_count > MAX_GSERVER_CLIENTS) {
            actual_client_count = MAX_GSERVER_CLIENTS;
        }

        if (actual_client_count < MIN_GSERVER_CLIENTS_FOR_GAME_START) {
            actual_client_count = MIN_GSERVER_CLIENTS_FOR_GAME_START;
        }

        server_set_max_clients(this->server, actual_client_count);
    }

    if (strcmp(config->name, current->name) != 0) {
        strcpy(this->server->name, config->name);
    }

    // We can only start the game when the server has more than 1 player connected
    if (config->start_game && this->server->current_clients >= MIN_GSERVER_CLIENTS_FOR_GAME_START) {
        this->status = GSS_GAME_IN_PROGRESS;
    } else {
        send_gserver_config_to_host(this); // Keep asking for more updates until they eventually start the game
    }

    update_gserver_info(this);
    this->info_changed = 1;
}

void send_winner_event(GServer *this, int client_id) {
    int *nargs = nargs_gameover();
    *nargs = client_id;
    NetEvent *winnerClient = net_event_new(GAME_OVER, nargs);
    server_send_event_to_all(this->server, winnerClient);

    this->status = GSS_SHUTTING_DOWN;
    this->server->current_clients = 0;
    update_gserver_info(this);
    this->info_changed = 1;
}

/*
    Handles all client NetEvents (i.e. playing the game, starting the game)

    PARAMS:
        GServer *this : the GServer
        int client_id : which client
        NetEvent *event : the NetEvent this client sent us

    RETURNS: none
*/
void gserver_handle_net_event(GServer *this, int client_id, NetEvent *event) {
    void *args = event->args;
    Server *server = this->server;

    switch (event->protocol) {

    case GSERVER_CONFIG:
        recv_gserver_config(this, client_id, event);
        break;

    case CARD_COUNT:
        int *arg = args;
        for (int i = 0; i < 4; i++) {
            if (this->all_clients[i] == client_id) {
                this->decks[i * 2 + 1] = arg[0];
            }
        }
        CardCountArray *cardcounts = nargs_card_count_array();
        for (int i = 0; i < 8; i++) {
            cardcounts[i] = this->decks[i];
        }
        if (arg[0] == 1) {
            this->data->currentUno = client_id;
            this->firstUNO = -1;
            int *nargs = nargs_uno();
            *nargs = client_id;
            NetEvent *uno = net_event_new(UNO, nargs);
            server_send_event_to_all(this->server, uno);
        }
        if (arg[0] == 0) {
            send_winner_event(this, client_id);
        } else {
            NetEvent *newEvent = net_event_new(CARD_COUNT, cardcounts);
            server_send_event_to_all(this->server, newEvent);
        }

        for (int i = 0; i < 4; i++) {
            if (this->all_clients[i] == client_id) {
                int next = (i + 1) % 4;
                while (this->all_clients[next] == -1 && next != i) {
                    next = (next + 1) % 4;
                }
                if (this->all_clients[next] != -1) {
                    this->data->client_id = this->all_clients[next];
                }
                break;
            }
        }
        break;

    case UNO:
        int *uno = args;
        if (this->firstUNO == -1) {
            this->firstUNO = uno[0];
            int *nargs = nargs_drawCards();
            *nargs = client_id;
            NetEvent *drawCards = net_event_new(DRAWCARDS, nargs);
            server_send_event_to_all(this->server, drawCards);
        }
    default:
        break;
    }
}

void get_host_client_id(GServer *this) {
    Client **clients = this->server->clients;
    // We have a host, and they are still connected.
    if (this->host_client_id > -1 && !clients[this->host_client_id]->recently_disconnected) {
        return;
    }

    Server *server = this->server;

    FOREACH_CLIENT(server) {
        // Our host disconnected while we're in the waiting phase. We must assign a new host to start the game
        if (client->recently_disconnected && this->host_client_id == client->id && this->status == GSS_WAITING_FOR_PLAYERS) {

            if (this->server->current_clients == 0) { // No point. The server should be shutting down.
                this->host_client_id = -1;
                return;
            }

            FOREACH_CLIENT(server) {
                // Get the first client and give them host permissions
                if (!client->recently_disconnected) {
                    this->host_client_id = client_id;

                    update_gserver_info(this);
                    this->info_changed = 1;

                    send_gserver_config_to_host(this);
                    break;
                }
            }
            END_FOREACH_CLIENT()

            break;
        }

        // The first person who joins this fresh server is the host
        if (this->status == GSS_WAITING_FOR_PLAYERS && this->server->current_clients == 1 && this->host_client_id == -1 && client->recently_connected) {
            this->host_client_id = client_id;

            update_gserver_info(this);
            this->info_changed = 1;

            break;
        }
    }
    END_FOREACH_CLIENT()
}

void gserver_shutdown(GServer *this) {
    GServerInfo *current_info = this->info_event->args;
    current_info->status = GSS_SHUTTING_DOWN;
    current_info->current_clients = 0;

    FOREACH_CLIENT((this->server)) {
        disconnect_client(client);
    }
    END_FOREACH_CLIENT()

    attach_event(this->cserver_send_queue, this->info_event);
    send_to_cserver(this);
}

/*
    The GServer loop.
    1) Update GServerInfo.
    2) Processes client NetEvents.
    3) Update the game state and queues any NetEvents that needed to be sent back to clients.
    4) Send any events to the CServer.

    PARAMS:
        GServer *this : the GServer

    RETURNS: none
*/
void gserver_loop(GServer *this) {
    Server *server = this->server;

    get_host_client_id(this);
    check_update_gserver_info(this);

    int current_clients = server->current_clients;

    // 1 player remaining and we're still in the game, so whoever is last standing wins
    if (current_clients == 1 && this->status == GSS_GAME_IN_PROGRESS) {
        int remainingId = -1;
        FOREACH_CLIENT(server) {
            if (client->recently_disconnected) {
                continue;
            }
            remainingId = client_id;
            break;
        }
        END_FOREACH_CLIENT()

        send_winner_event(this, remainingId);
    }

    // Newly connected clients
    FOREACH_CLIENT(server) {
        if (client->recently_connected) {
            for (int i = 0; i < 4; i++) {
                if (this->all_clients[i] == -1) {
                    this->all_clients[i] = client_id;
                    this->decks[i * 2] = client_id;
                    break;
                }
            }
            int *nargs = nargs_shmid();
            *nargs = this->SERVERSHMID;
            NetEvent *sendShmid = net_event_new(SHMID, nargs);
            server_send_event_to(this->server, client_id, sendShmid);

            // Set the host to the first person who joined
            if (this->status == GSS_WAITING_FOR_PLAYERS && this->server->current_clients > 0 && this->host_client_id == -1) {
                this->host_client_id = 0;

                update_gserver_info(this);
                this->info_changed = 1;

                send_gserver_config_to_host(this);
            }
        }
    }
    END_FOREACH_CLIENT()

    FOREACH_CLIENT(server) { // Each player's events
        NetEventQueue *queue = client->recv_queue;
        for (int i = 0; i < queue->event_count; ++i) {
            gserver_handle_net_event(this, client_id, queue->events[i]);
        }
    }
    END_FOREACH_CLIENT()
}

static void handle_sigint(int signo) {
    if (signo != SIGINT) {
        return;
    }

    kill(connection_handler_pid, SIGINT);
    exit(EXIT_SUCCESS);
}

/*
    Starts the GServer. It will accept clients and be able to send / receive events.

    PARAMS:
        GServer *this : the GServer

    RETURNS: none
*/
void gserver_run(GServer *this) {
    signal(SIGINT, handle_sigint);

    Server *server = this->server;
    this->status = GSS_RESERVED;
    srand(getpid());
    this->SERVERSHMID = rand();
    int shmid;
    shmid = shmget(this->SERVERSHMID, sizeof(gameState), IPC_CREAT | 0666);
    this->data = shmat(shmid, 0, 0);
    this->data->lastCard = generate_card();
    this->data->client_id = 0;

    server_start_connection_handler(server);
    connection_handler_pid = server->connection_handler_pid;

    int counter = 0;

    while (1) {
        handle_connections(server);

        server_empty_recv_events(server);
        server_recv_events(server);

        gserver_loop(this);

        server_send_events(server);
        send_to_cserver(this);
        usleep(TICK_TIME_MICROSECONDS);
    }
}
