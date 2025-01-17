// #include <SDL2/SDL.h>
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

// #define SHMID 123456789

typedef enum ClientState ClientState;
enum ClientState {
    IN_CSERVER,
    IN_GSERVER,
};

ClientState client_state;
GServerInfoList *gservers; // Global server list
int connected_gserver_id = -1;
int others[4];
int SERVERSHMID;

void connect_to_gserver(BaseClient *gclient, GServerInfo *server_info) {
    int connected_to_gserver = client_connect(gclient, server_info->wkp_name);
    if (connected_to_gserver == -1) {
        printf("[CLIENT]: Failed to connect to GServer\n");
        return;
    }
    connected_gserver_id = server_info->id;
    client_state = IN_GSERVER;
}

void connect_to_cserver(BaseClient *cclient) {
    int connected_to_cserver = client_connect(cclient, CSERVER_WKP_NAME);
    if (connected_to_cserver == -1) {
        printf("[CLIENT]: Failed to establish connection with the central server\n");
        exit(EXIT_FAILURE);
    }
    client_state = IN_CSERVER;
}

void print_gserver_list(GServerInfoList *recv_gserver_list) {
    // Don't print the server list if we're in a game server.
    if (client_state == IN_GSERVER) {
        return;
    }

    printf("======= Game Server List\n");

    int reserved_server_count = 0;
    for (int i = 0; i < MAX_CSERVER_GSERVERS; ++i) {
        GServerInfo *info = recv_gserver_list[i];
        if (info->status != GSS_UNRESERVED) {
            reserved_server_count++;
        }
    }
    printf("Available game servers: %d\n", reserved_server_count);

    for (int i = 0; i < MAX_CSERVER_GSERVERS; ++i) {
        GServerInfo *info = recv_gserver_list[i];

        if (info->status == GSS_UNRESERVED) { // Don't display unreserved servers
            continue;
        }

        char status[100];
        switch (info->status) {

        case GSS_RESERVED:
            strcpy(status, "RESERVED");
            break;

        case GSS_WAITING_FOR_PLAYERS:
            strcpy(status, "WAITING FOR PLAYERS");
            break;

        case GSS_GAME_IN_PROGRESS:
            strcpy(status, "GAME IN PROGRESS");
            break;

        default:
            break;
        }

        printf("[%d] %s: %d / %d (%s)\n", info->id, info->name, info->current_clients, info->max_clients, status);
    }
    printf("========================\n");
}

void handle_cserver_net_event(BaseClient *cclient, BaseClient *gclient, NetEvent *event) {
    void *args = event->args;

    switch (event->protocol) {

    case GSERVER_LIST: { // Print server list
        print_gserver_list(args);
        break;
    }

    case RESERVE_GSERVER: { // The CServer has given us the GServer to join
        ReserveGServer *nargs = args;
        int gserver_id = nargs->gserver_id;

        if (gserver_id == -1) {
            printf("Crap! Can't join any servers\n");
            return;
        }

        GServerInfo *server_info = gservers[gserver_id];
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

        GServerInfo *gserver_info = gservers[which_server];
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

void disconnect_from_gserver(BaseClient *client) {
    if (client_state != IN_GSERVER) {
        return;
    }

    client_disconnect_from_server(client);
    client_state = IN_CSERVER;
}

void handle_gserver_net_event(BaseClient *client, NetEvent *event) {
    void *args = event->args;

    // Run game logic + rendering based on NetEvents HERE
    switch (event->protocol) {
    case CARD_COUNT:
        int *arg = args;
        printf("client %d has %d cards, client %d has %d cards\n", arg[0], arg[1], arg[2], arg[3]);
        break;

    case SHMID:
        int *shmid = args;
        printf("client recieved shmid: %d\n", *shmid);
        SERVERSHMID = *shmid;
        break;

    case GAME_OVER:
        int *winner = args;
        printf("client %d has won\n", winner[0]);
        break;

    case GSERVER_CONFIG: // We're the host!
        GServerConfig *config = args;

        GServerConfig *new_config = nargs_gserver_config();
        memcpy(new_config, config, sizeof(GServerConfig));

        NetEvent *send_config_event = net_event_new(GSERVER_CONFIG, new_config);

        printf("YOU ARE THE HOST! Edit the server with: c {n} to set server to n max clients; s to start the game\n");

        char input[100];
        fgets(input, sizeof(input), stdin);
        switch (input[0]) {

        case 'c':
            int max_clients;
            sscanf(input + 1, "%d", &max_clients);
            new_config->max_clients = max_clients;

            break;

        case 's':
            new_config->start_game = 1;
            break;

        case 'd':
            disconnect_from_gserver(client);
            connected_gserver_id = -1;
            return;

        default:
            break;
        }

        client_send_event(client, send_config_event);
        break;

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
    connect_to_cserver(cclient);

    BaseClient *gclient = client_new(username);

    // fcntl(STDIN_FILENO, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);

    // Set up the networked server list
    gservers = nargs_gserver_info_list();
    NetEvent *info_list_event = net_event_new(GSERVER_LIST, gservers);
    attach_event(cclient->recv_queue, info_list_event);

    // Game stuff (should be in a separate function)
    srand(getpid());

    card deck[100];
    int others = 0;
    int num_cards = 7;
    generate_cards(deck, num_cards);
    char input[10];
    gameState *data;
    int shmid = 0;

    /*
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL_Error: %s\n", SDL_GetError());
        return;
    }
    */

    // SDL_Window *window = SDL_CreateWindow("Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_UNDEFINED, 800, 800, SDL_WINDOW_SHOWN);
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

            if (shmid == 0) {
                shmid = shmget(SERVERSHMID, sizeof(gameState), 0);
                data = shmat(shmid, 0, 0);
            }

            if (data->client_id == gclient->client_id && gservers[connected_gserver_id]->status == GSS_GAME_IN_PROGRESS) {
                printf("gamestate card:%d gamestate color: %d gamestate turn:%d\n", data->lastCard.num, data->lastCard.color, data->client_id);
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
                    if (picked.num == data->lastCard.num || picked.color == data->lastCard.color) {
                        data->lastCard = picked;
                        play_card(deck, picked, num_cards);
                        num_cards--;
                    }
                }
                CardCountArray *cardcounts = nargs_card_count_array();
                cardcounts[0] = num_cards;
                NetEvent *card_counts = net_event_new(CARD_COUNT, cardcounts);
                client_send_event(gclient, card_counts);
            }

            client_send_to_server(gclient);

            // TEMP DISCONNECT INPUT
            if (input[0] == 'D') {
                disconnect_from_gserver(gclient);
                connected_gserver_id = -1;
                continue;
            }
        }

        client_send_to_server(cclient);
        usleep(TICK_TIME_MICROSECONDS);
    }
}
