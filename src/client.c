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
#include "sdl/gserverwaitui.h"
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
int width = 800;//Width of SDL
int height = 800;//Height of SDL
SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *textures[100];//Textures for numbers 0-99;
int num_cards = 7;
gameState *data;//Struct for game info that will be stored in shared memory
int others[8] = {7, 7, 7, 7, 7, 7, 7, 7};//Array to store other client info, client_id and number of cards
int shmid = 0;
card deck[100];//Array of cards that the client has
int unoCalled = 0;//Display uno button if true
int drawUno = 0;//Draw cards if failed uno

GServerConfig *currentConfig;

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

void handle_cserver_net_event(BaseClient *cclient, BaseClient *gclient, NetEvent *event) {
    void *args = event->args;

    switch (event->protocol) {

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
    memset(input, 0, sizeof(input));
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

//Disconnects from game and detaches shared memory and resets variables
//Will probably break if there's more than 2 people
void disconnectSDL(BaseClient *gclient) {
    shmdt(data);
    shmctl(shmid, IPC_RMID, 0);
    shmid = 0;
    num_cards = 7;

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

    case SHMID://Receives shmid from server
        int *shmid = args;
        SERVERSHMID = *shmid;
        break;

    case UNO://Receives notification that someone has 1 card
        int *uno = args;
        // printf("%d has uno.\n", uno[0]);
        unoCalled = 1;//Uno variable that controls if uno button shows up
        break;

    case DRAWCARDS:
        int *draw = args;
        // printf("%d called uno.\n", draw[0]);
        if (data->currentUno == client->client_id) {
            if (draw[0] != client->client_id) {
                drawUno = 1;
                data->currentUno = -1;
            }
        }
        unoCalled = 0;
        break;

    case GAME_OVER:
        int *winner = args;
        printf("CLIENT %d HAS WON\n", winner[0]);
        if (*winner == client->client_id) {
            printf("THAT'S YOU! CONGRATULATIONS, UNO CHAMPION!!!\n");
        }
        disconnectSDL(client);
        break;

    case GSERVER_CONFIG:
        GServerConfig *config = args;
        memcpy(currentConfig, config, sizeof(GServerConfig));
        break;

    default:
        break;
    }
}

void handleInputForGServerWait(GServerInfo *serverInfo, BaseClient *gclient, int action) {
    if (action == GSERVER_WAITING_NOTHING) {
        return;
    }

    if (action == GSERVER_WAITING_DISCONNECT) {
        disconnect_from_gserver(gclient);
        return;
    }

    NetEvent *send_config_event = net_event_new(GSERVER_CONFIG, currentConfig);
    send_config_event->cleanup_behavior = NEVENT_PERSISTENT_ARGS; // Remove this to see a LEGENDARY malloc error

    if (action == GSERVER_WAITING_START_GAME) {
        if (serverInfo->current_clients > 1) {
            currentConfig->start_game = 1;
        } else {
            printf("Can't start the game! There needs to be at least 2 players connected\n");
            return;
        }
    }

    // Change max clients
    currentConfig->max_clients = action;
    client_send_event(gclient, send_config_event);
}

static void sighandler(int signo) {
    if (signo == SIGINT) {
        TTF_Quit();
        SDL_Quit();
        printf("EXIT\n");
        exit(EXIT_SUCCESS);
    }
}
//Client mouse input, determines actions and what they do
int actions(card *deck, BaseClient *gclient, int action) {
    if (drawUno == 1) {//Draw two cards if failed uno
        deck[num_cards] = add_card(deck, num_cards, width, height);
        num_cards++;
        deck[num_cards] = add_card(deck, num_cards, width, height);
        num_cards++;
        drawUno = 0;
        return 2;
    }

    if (action == -2) { // Draw cards
        deck[num_cards] = add_card(deck, num_cards, width, height);
        num_cards++;
        return 2;
    }

    if (action == -4) {//Clicked uno button
        return -4;
    }
    card picked = deck[action]; // Play card
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

    // Set up the networked server list
    gservers = nargs_gserver_info_list();
    NetEvent *info_list_event = net_event_new(GSERVER_LIST, gservers);
    attach_event(cclient->recv_queue, info_list_event);

    currentConfig = nargs_gserver_config();

    char windowName[100];
    snprintf(windowName, sizeof(windowName), "Game (%s)", username);

    //Initializes game for SDL rendering
    window = SDL_CreateWindow(windowName, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDLInitText(textures, renderer);

    srand(getpid());

    while (1) {
        client_recv_from_server(cclient);
        if (!client_is_connected(cclient)) {
            sighandler(SIGINT);
        }

        for (int i = 0; i < cclient->recv_queue->event_count; ++i) {
            NetEvent *event = cclient->recv_queue->events[i];
            handle_cserver_net_event(cclient, gclient, event);
        }

        if (client_state == IN_CSERVER && cclient->client_id >= 0) {
            renderServerList(renderer, gservers);
            int action = handleServerListEvent();
            handleInputForCServer(cclient, gclient, action);
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
                if (shmid == 0) { //Gets shared memory once
                    shmid = shmget(SERVERSHMID, sizeof(gameState), 0);
                    data = shmat(shmid, 0, 0);
                    generate_cards(deck, num_cards, width, height);
                }
                modCoords(deck, num_cards);
                render(renderer, textures, deck, num_cards, data, others, gclient->client_id, unoCalled, gclient);

                //Checks user mouse input
                SDL_Event e;
                int action = EventPoll(e, deck, num_cards);
                if (action == -3) { // Always check for disconnects even when its not our turn
                    disconnectSDL(gclient);
                    printf("Disconnected from game\n");
                    continue;
                }
                if (action == -4 && unoCalled) { // Always check for calling Uno
                    int *unoEvent = nargs_uno();
                    *unoEvent = gclient->client_id;
                    NetEvent *uno = net_event_new(UNO, unoEvent);
                    client_send_event(gclient, uno);
                    unoCalled = 0;
                }

                //Checks if it's the client's turn then does actions
                if (data->client_id == gclient->client_id) {
                    int input = actions(deck, gclient, action);

                    if (input != 0) {
                        CardCountArray *cardcounts = nargs_card_count_array();
                        cardcounts[0] = num_cards;
                        NetEvent *card_counts = net_event_new(CARD_COUNT, cardcounts);
                        client_send_event(gclient, card_counts);
                    }
                }
            } else if (gservers[connected_gserver_id]->status == GSS_WAITING_FOR_PLAYERS) {
                renderGServerWait(renderer, gservers[connected_gserver_id], gclient);
                int action = handleGServerWaitEvent(gservers[connected_gserver_id], gclient);
                handleInputForGServerWait(gservers[connected_gserver_id], gclient, action);
            }

            client_send_to_server(gclient);
        }

        client_send_to_server(cclient);
        usleep(TICK_TIME_MICROSECONDS);
    }
}
