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
#include "client/gserverlist.h"

#define SHMID 123456789
// #define DONT

GServerList *gserver_list;

void handle_cserver_net_event(BaseClient *client, NetEvent *event) {
    void *args = event->args;

    switch (event->protocol) {

    case GSERVER_LIST: {
        printf("mhm\n");
        GServerInfoList *nargs = args;
        GServerInfo **recv_gserver_list = nargs->gserver_list;

        int did_change = update_gserver_list(gserver_list, recv_gserver_list);
        if (did_change) {
            print_gserver_list(gserver_list);
        }
    }

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
    srand(getpid());
    int shmid;
    card *data;
    shmid = shmget(SHMID, sizeof(card), IPC_CREAT | 0640);
    data = shmat(shmid, 0, 0);

    gserver_list = gserver_list_new();

#ifdef DONT
    BaseClient *cclient = client_new();
    int connected_to_cserver = client_connect(cclient, CSERVER_WKP_NAME);
    if (connected_to_cserver == -1) {
        printf("[CLIENT]: Failed to establish connection with the central server\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
        client_recv_from_server(cclient);
        for (int i = 0; i < cclient->recv_queue->event_count; ++i) {
            NetEvent *event = cclient->recv_queue->events[i];
            handle_cserver_net_event(cclient, event);
        }

        client_send_to_server(cclient);
        usleep(TICK_TIME_MICROSECONDS);
    }
#endif

#ifndef DONT
    BaseClient *gclient = client_new();
    int connected_to_gserver = client_connect(gclient, "G69");

    card deck[100];
    int num_cards = 7;
    generate_cards(deck, num_cards);
    // Should be done by server on setup not by client
    *data = generate_card();
    char input[10];

    while (1) {
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

        for (int i = 0; i < 1; ++i) {
            NetArgs_PeriodicHandshake *test_args = malloc(sizeof(NetArgs_PeriodicHandshake));
            test_args->id = rand();

            printf("rand: %d\n", test_args->id);
            NetEvent *test_event = net_event_new(PERIODIC_HANDSHAKE, test_args);

            client_send_event(gclient, test_event);
        }

        client_send_to_server(gclient);

        usleep(TICK_TIME_MICROSECONDS);
    }
#endif
}
