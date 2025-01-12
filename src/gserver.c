#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "gserver.h"
#include "network/pipehandshake.h"
#include "network/pipenet.h"
#include "network/pipenetevents.h"
#include "shared.h"

GServer *gserver_new(int id) {
    GServer *this = malloc(sizeof(GServer));
    this->status = GSS_WAITING_FOR_PLAYERS;

    this->server = server_new(id);
    this->server->max_clients = DEFAULT_GSERVER_MAX_CLIENTS;

    char gserver_name_buffer[MAX_GSERVER_NAME_CHARACTERS];
    snprintf(gserver_name_buffer, sizeof(gserver_name_buffer), "GameServer%d", id);
    strcpy(this->server->name, gserver_name_buffer);

    char gserver_wkp_name_buffer[GSERVER_WKP_NAME_LEN];
    snprintf(gserver_wkp_name_buffer, sizeof(gserver_wkp_name_buffer), "G%d", id);
    strcpy(this->server->wkp_name, gserver_wkp_name_buffer);

    return this;
}

// HANDLE CLIENT EVENTS (i.e. change game state) HERE
void gserver_handle_net_event(GServer *this, int client_id, NetEvent *event) {
    void *args = event->args;

    switch (event->protocol) {
        case CARD_COUNT:
            break;
    default:
        break;
    }
}

// ALL GAME LOGIC GOES HERE
void gserver_loop(GServer *this) {
    Server *server = this->server;

    FOREACH_CLIENT(server) {
        NetEventQueue *queue = client->recv_queue;
        for (int i = 0; i < queue->event_count; ++i) {
            gserver_handle_net_event(this, client_id, queue->events[i]);
        }
    }
    END_FOREACH_CLIENT()

    /* BELOW IS A TEST FOR SERVER SEND
    NetArgs_PeriodicHandshake *test_args = malloc(sizeof(NetArgs_PeriodicHandshake));
    test_args->id = rand();

    printf("rand: %d\n", test_args->id);
    NetEvent *test_event = net_event_new(PERIODIC_HANDSHAKE, test_args);

    server_send_event_to_all(server, test_event);
    */
    /*
    this->decks[0] = 1;
    this->decks[1] = 1;
    NetArgs_CardCounts *test_args = malloc(sizeof(NetArgs_CardCounts));
    memcpy(test_args->decks, this->decks, sizeof(this->decks));

    NetEvent *test_event = net_event_new(CARD_COUNT, test_args);

    server_send_event_to_all(server, test_event);*/
}

void gserver_run(GServer *this) {
    Server *server = this->server;

    server_start_connection_handler(server);

    while (1) {
        handle_connections(server);

        server_empty_recv_events(server);
        server_recv_events(server);

<<<<<<< HEAD
        gserver_loop(this);
=======
        while (1) {
            bytes_read = read(recv_fd, &client_id, sizeof(client_id));
            if (bytes_read <= 0) {
                break;
            }

            char *event_buffer = read_into_buffer(recv_fd);
            recv_event_queue(recv_queue, event_buffer);

            printf("ID: %d | Count: %d \n", client_id, recv_queue->event_count);

            // ALL of these events come from client_id
            for (int i = 0; i < recv_queue->event_count; ++i) {

                NetEvent *event = recv_queue->events[i];
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
                    printf("client %d connected \n", client_id);
                    break;
                }
                case CARD_COUNT:{
                    NetArgs_CardCount *nargs = args;
                    printf("sent card\n");
                    break;
                }
                default:
                    break;
                }
            }
            printf("loop done\n");
        }
>>>>>>> 80b08123f68df61ed682db03f9c0500769fc2aef

        server_send_events(server);
        usleep(TICK_TIME_MICROSECONDS);
    }
}
