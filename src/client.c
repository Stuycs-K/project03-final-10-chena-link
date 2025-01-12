#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>

#include "client.h"
#include "game.h"
#include "network/pipehandshake.h"
#include "network/pipenet.h"
#include "network/pipenetevents.h"
#include "shared.h"
#include "util/file.h"

#include "client/baseclient.h"

#define SHMID 123456789
#define DONT

typedef enum ClientState ClientState;
enum ClientState {
    IN_CSERVER,
    IN_GSERVER,
};

ClientState client_state;
GServerInfoList *gservers; // Global server list

void connect_to_gserver(BaseClient *gclient, GServerInfo *server_info) {
    int connected_to_gserver = client_connect(gclient, server_info->wkp_name);
    if (connected_to_gserver == -1) {
        printf("[CLIENT]: Failed to connect to GServer\n");
        return;
    }
    client_state = IN_GSERVER;
}

void print_gserver_list(GServerInfoList *nargs) {
    // Don't print the server list if we're in a game server.
    if (client_state == IN_GSERVER) {
        return;
    }

    printf("======= Game Server List\n");
    GServerInfo **recv_gserver_list = nargs->list;

    for (int i = 0; i < MAX_CSERVER_GSERVERS; ++i) {
        GServerInfo *info = recv_gserver_list[i];
        /*
        if (info->status == 1) {
            continue;
        }
        */
        char status[100];
        switch (info->status) {

        case GSS_UNRESERVED:
            strcpy(status, "UNRESERVED");
            break;

        case GSS_WAITING_FOR_PLAYERS:
            strcpy(status, "WAITING FOR PLAYERS");
            break;

        default:
            break;
        }

        printf("[%d] %s: %d / %d (%s)\n", info->id, info->name, info->current_clients, info->max_clients, status);
    }
}

void handle_cserver_net_event(BaseClient *cclient, BaseClient *gclient, NetEvent *event) {
    void *args = event->args;

    switch (event->protocol) {

    case GSERVER_LIST: { // Print server list
        GServerInfoList *nargs = args;
        print_gserver_list(nargs);
        break;
    }

    case RESERVE_GSERVER: { // The CServer has given us the GServer to join
        ReserveGServer *nargs = args;
        int gserver_id = nargs->gserver_id;

        if (gserver_id == -1) {
            printf("Crap! Can't join any servers\n");
            return;
        }

        GServerInfo *server_info = gservers->list[gserver_id];
        printf("%s\n", server_info->wkp_name);

        connect_to_gserver(gclient, server_info);
        break;
    }

    default:
        break;
    }
}

char *get_username() {
    char *username = calloc(sizeof(char), MAX_PLAYER_NAME_CHARACTERS);

    char input[MAX_PLAYER_NAME_CHARACTERS + 1];
    printf("Enter your username (%d characters maximum):\n", MAX_PLAYER_NAME_CHARACTERS - 1);

    fgets(input, sizeof(input), stdin);

    // Remove the newline, if it exists
    input[strcspn(input, "\n")] = 0;

    if (strlen(input) > MAX_PLAYER_NAME_CHARACTERS - 1) {
        printf("Name is too long\n");
        return NULL;
    }

    strncpy(username, input, MAX_PLAYER_NAME_CHARACTERS);

    /*
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {
    }
    */

    printf("\n\n");

    return username;
}

void input_for_cserver(BaseClient *client, BaseClient *gclient) {
    if (client_state == IN_GSERVER) {
        return;
    }

    printf("TYPE c TO CREATE AND JOIN A SERVER. TYPE j {n} WHERE n IS A VISIBLE SERVER ID TO JOIN A SERVER\n");
    char input[256];
    fgets(input, sizeof(input), stdin);

    char option = input[0];
    switch (option) {

    case 'c':
        NetEvent *reserve_event = net_event_new(RESERVE_GSERVER, nargs_reserve_gserver());
        insert_event(client->send_queue, reserve_event);
        break;

    case 'j':
        int which_server;
        sscanf(input, "j %d", &which_server);

        GServerInfo *gserver_info = gservers->list[which_server];
        GServerStatus status = gserver_info->status;

        // Server must be in the waiting for players phase and not be full
        if (status == GSS_WAITING_FOR_PLAYERS || gserver_info->current_clients >= gserver_info->max_clients) {
            connect_to_gserver(gclient, gserver_info);
        } else {
            printf("YOU CAN'T JOIN THAT ONE!\n");
        }

        break;

    default:
        break;
    }
}

void handle_gserver_net_event(BaseClient *client, NetEvent *event) {
    void *args = event->args;

    // Run game logic + rendering based on NetEvents HERE
    switch (event->protocol) {

    case PERIODIC_HANDSHAKE: {
        NetArgs_PeriodicHandshake *nargs = args;
        printf("we GOT from server: %d\n", nargs->id);
        break;
    }

    default:
        break;
    }
}

void client_main(void) {
    client_state = IN_CSERVER;
    char *username = get_username();
    if (username == NULL) {
        exit(EXIT_FAILURE);
    }

    // First, try to connect to the central server
    BaseClient *cclient = client_new(username);
    int connected_to_cserver = client_connect(cclient, CSERVER_WKP_NAME);
    if (connected_to_cserver == -1) {
        printf("[CLIENT]: Failed to establish connection with the central server\n");
        exit(EXIT_FAILURE);
    }

    BaseClient *gclient = client_new(username);

    // Set up the networked server list
    gservers = nargs_gserver_info_list();
    NetEvent *info_list_event = net_event_new(GSERVER_LIST, gservers);
    attach_event(cclient->recv_queue, info_list_event);

    // Game stuff (should be in a separate function)
    srand(getpid());
    int shmid;
    card *data;
    shmid = shmget(SHMID, sizeof(card), IPC_CREAT | 0640);
    data = shmat(shmid, 0, 0);

    card deck[100];
    int num_cards = 7;
    generate_cards(deck, num_cards);
    // Should be done by server on setup not by client
    *data = generate_card();
    char input[10];

    while (1) {
        // 1) Receive NetEvents from CServer
        client_recv_from_server(cclient);
        for (int i = 0; i < cclient->recv_queue->event_count; ++i) {
            NetEvent *event = cclient->recv_queue->events[i];
            handle_cserver_net_event(cclient, gclient, event);
        }

        if (cclient->client_id >= 0) {
            input_for_cserver(cclient, gclient);
        }

        // 3) If we connected to a GServer, play the game!
        if (client_is_connected(gclient)) {
            client_recv_from_server(gclient);
            for (int i = 0; i < gclient->recv_queue->event_count; ++i) {
                NetEvent *event = gclient->recv_queue->events[i];
                handle_gserver_net_event(gclient, event);
            }

            if (gclient->client_id < 0) {
                continue;
            }

            printf("Current card: Color: %d Num: %d\n", data->color, data->num);
            for (int i = 0; i < num_cards; i++) {
                printf("%d: color: %d num: %d\n", i, deck[i].color, deck[i].num);
            }
            fgets(input, sizeof(input), stdin);
            if (input[0] == 'l') {
                deck[num_cards] = generate_card();
                num_cards++;
            }
            if (input[0] == 'p') {
                card picked;
                int col;
                int num;
                sscanf(input + 1, "%d %d", &col, &num);
                picked.color = col;
                picked.num = num;
                if (picked.num == data->num || picked.color == data->color) {
                    *data = picked;
                    play_card(deck, picked, num_cards);
                    num_cards--;
                }
            }

            /*
            for (int i = 0; i < 1; ++i) {
                NetArgs_PeriodicHandshake *test_args = malloc(sizeof(NetArgs_PeriodicHandshake));
                test_args->id = rand();

                printf("rand: %d\n", test_args->id);
                NetEvent *test_event = net_event_new(PERIODIC_HANDSHAKE, test_args);

                client_send_event(gclient, test_event);
            }
            */

            client_send_to_server(gclient);
        }

        client_send_to_server(cclient);
        usleep(TICK_TIME_MICROSECONDS);
    }
}
