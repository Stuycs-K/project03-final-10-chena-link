#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
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
#include "sdl/SDL.h"
#include "sdl/serverlistui.h"
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
int SERVERSHMID;
int width = 800;
int height = 800;
SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *textures[100];
int num_cards = 7;
gameState *data;
int others[8] = {7, 7, 7, 7, 7, 7, 7, 7};
int shmid = 0;
card deck[100];
int unoCalled = 0;
int drawUno = 0;

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

void handleInputForCServer(BaseClient *client, BaseClient *gclient, int action) {
    if (client_state == IN_GSERVER) {
        return;
    }

    if (action == SERVER_LIST_EVENT_NOTHING) {
        return;
    }

    if (action == SERVER_LIST_EVENT_RESERVE) {
        NetEvent *reserve_event = net_event_new(RESERVE_GSERVER, nargs_reserve_gserver());
        insert_event(client->send_queue, reserve_event);
    } else {
        GServerInfo *gserver_info = gservers[action];
        GServerStatus status = gserver_info->status;

        // Server must be in the waiting for players phase and not be full
        if (status == GSS_WAITING_FOR_PLAYERS && gserver_info->current_clients < gserver_info->max_clients) {
            connect_to_gserver(gclient, gserver_info);
        } else {
            printf("YOU CAN'T JOIN THAT ONE!\n");
        }
    }
}

void disconnect_from_gserver(BaseClient *client) {
    if (client_state != IN_GSERVER) {
        return;
    }

    if (client_is_connected(client)) {
        client_disconnect_from_server(client);
    }

    connected_gserver_id = -1;
    client_state = IN_CSERVER;
}

void disconnectSDL(BaseClient *gclient) {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    shmdt(data);
    shmctl(shmid, IPC_RMID, 0);
    shmid = 0;
    num_cards = 7;
    TTF_Quit();
    SDL_Quit();
    disconnect_from_gserver(gclient);
}

void handle_gserver_net_event(BaseClient *client, NetEvent *event) {
    void *args = event->args;

    // Run game logic + rendering based on NetEvents HERE
    switch (event->protocol) {
    case CARD_COUNT:
        int *arg = args;
        for (int i = 0; i < 8; i++) {
            others[i] = arg[i];
        }
        // printf("client %d has %d cards, client %d has %d cards\n", arg[0], arg[1], arg[2], arg[3]);
        break;

    case SHMID:
        int *shmid = args;
        // printf("client recieved shmid: %d\n", *shmid);
        SERVERSHMID = *shmid;
        break;

    case UNO:
        int *uno = args;
        printf("%d has uno.\n", uno[0]);
        unoCalled = 1;
        break;

    case DRAWCARDS:
        int *draw = args;
        printf("%d called uno.\n", draw[0]);
        if (data->currentUno == client->client_id) {
            if (draw[0] != client->client_id) {
                drawUno = 1;
            }
        }
        unoCalled = 0;
        break;

    case GAME_OVER:
        int *winner = args;
        printf("client %d has won\n", winner[0]);
        disconnectSDL(client);
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

static void sighandler(int signo) {
    if (signo == SIGINT) {
        printf("SIGINT received\n");
        TTF_Quit();
        SDL_Quit();
        exit(0);
    }
}

int actions(card *deck, BaseClient *gclient) {
    if (drawUno == 1) {
        deck[num_cards] = add_card(deck, num_cards, width, height);
        num_cards++;
        deck[num_cards] = add_card(deck, num_cards, width, height);
        num_cards++;
        drawUno = 0;
        return 2;
    }
    SDL_Event e;
    int action = EventPoll(e, deck, num_cards);
    if (action == -2) {
        deck[num_cards] = add_card(deck, num_cards, width, height);
        num_cards++;
        return 2;
    }
    if (action == -3) {
        disconnectSDL(gclient);
        printf("disconnected from game\n");
        return 3;
    }
    if (action == -4) {
        return -4;
    }
    card picked = deck[action];
    if (action != -1) {
        if (picked.num == data->lastCard.num || picked.color == data->lastCard.color) {
            data->lastCard = picked;
            play_card(deck, picked, num_cards);
            num_cards--;
            return 1;
        }
    }
    return 0;
}

void client_main(void) {
    signal(SIGINT, sighandler);

    printf("LOADING ... \n");

    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    SDLInit();

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

    window = SDL_CreateWindow("Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDLInitText(textures, renderer);

    // Game stuff (should be in a separate function)
    srand(getpid());

    while (1) {
        // 1) Receive NetEvents from CServer
        client_recv_from_server(cclient);
        if (!client_is_connected(cclient)) {
            exit(EXIT_SUCCESS);
        }

        for (int i = 0; i < cclient->recv_queue->event_count; ++i) {
            NetEvent *event = cclient->recv_queue->events[i];
            handle_cserver_net_event(cclient, gclient, event);
        }

        if (client_state == IN_CSERVER) {
            renderServerList(renderer, gservers);
            int action = handleServerListEvent();
            handleInputForCServer(cclient, gclient, action);
        }

        if (cclient->client_id >= 0) {
            // input_for_cserver(cclient, gclient);
        }

        if (client_is_connected(gclient)) {
            client_recv_from_server(gclient);
            if (!client_is_connected(gclient)) {
                disconnect_from_gserver(gclient);
                continue;
            }

            for (int i = 0; i < gclient->recv_queue->event_count; ++i) {
                NetEvent *event = gclient->recv_queue->events[i];
                handle_gserver_net_event(gclient, event);
            }

            if (gclient->client_id < 0) {
                continue;
            }

            if (gservers[connected_gserver_id]->status == GSS_GAME_IN_PROGRESS) {
                if (shmid == 0) {
                    shmid = shmget(SERVERSHMID, sizeof(gameState), 0);
                    data = shmat(shmid, 0, 0);
                    generate_cards(deck, num_cards, width, height);
                }
                modCoords(deck, num_cards);
                render(renderer, textures, deck, num_cards, data->lastCard, others, gclient->client_id, unoCalled, gclient);

                if (data->client_id == gclient->client_id) {
                    int input = actions(deck, gclient);
                    if (input != 0) {
                        if (input == -4) {
                            int *unoEvent = nargs_uno();
                            *unoEvent = gclient->client_id;
                            NetEvent *uno = net_event_new(UNO, unoEvent);
                            client_send_event(gclient, uno);
                            unoCalled = 0;
                        } else {
                            CardCountArray *cardcounts = nargs_card_count_array();
                            cardcounts[0] = num_cards;
                            NetEvent *card_counts = net_event_new(CARD_COUNT, cardcounts);
                            client_send_event(gclient, card_counts);
                        }
                    }
                }
            }

            client_send_to_server(gclient);
        }

        client_send_to_server(cclient);
        usleep(TICK_TIME_MICROSECONDS);
    }
}
