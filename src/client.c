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

NetEvent *create_client_connect_event() {
    NetArgs_ClientConnect *nargs = malloc(sizeof(NetArgs_ClientConnect));
    nargs->name = calloc(sizeof(char), 20);

    return net_event_new(CLIENT_CONNECT, nargs);
}

void free_client_connect_event(NetEvent *event) {
    NetArgs_ClientConnect *nargs = event->args;
    free(nargs->name);
    free(event->args);
    free(event);
}

NetEvent *try_connect_to_server() {
    NetEvent *handshake_event = create_handshake_event();
    NetArgs_Handshake *handshake = handshake_event->args;

    client_setup("TEMP", handshake_event);
    int succeeded = client_handshake(handshake_event);

    if (succeeded == -1) {
        printf("Connection to server failed\n");
        return NULL;
    }

    return handshake_event;
}

#define SHMID 1234567890

void client_main(void) {
    srand(getpid());
    int shmid;
    card *data;
    shmid = shmget(SHMID, sizeof(card), IPC_CREAT | 0640);
    data = shmat(shmid, 0, 0);

    ClientList *gserver_client_list = nargs_client_list();
    NetEvent *gserver_client_list_event = net_event_new(CLIENT_LIST, gserver_client_list);
    gserver_client_list_event->is_persistent = 1;

    NetEvent *client_connect_event = create_client_connect_event();
    NetArgs_ClientConnect *client_connect = client_connect_event->args;

    // Everything below here will be looped in the future!
    NetEvent *handshake_event = try_connect_to_server();
    if (handshake_event == NULL) {
        return;
    }
    NetArgs_Handshake *handshake = handshake_event->args;

    int client_id = handshake->client_id;
    int to_server = handshake->client_to_server_fd;
    int from_server = handshake->server_to_client_fd;

    printf("This is %d, sending to %d\n", client_id, to_server);

    // First packet we send is a confirmation of our connection
    client_connect->to_client_fd = from_server;
    send_event_immediate(client_connect_event, to_server);

    NetEventQueue *send_queue = net_event_queue_new();
    NetEventQueue *recv_queue = net_event_queue_new();

    // SET RECEIVE FO TO NONBLOCK MODE!
    set_nonblock(from_server);

    card deck[100];
    int num_cards = 7;
    generate_cards(deck, num_cards);
    // Should be done by server on setup not by client
    *data = generate_card();
    char input[10];

    while (1) {
        // First: receive events
        empty_net_event_queue(recv_queue);

        void *recv_buffer;
        while (recv_buffer = read_into_buffer(from_server)) {
            recv_event_queue(recv_queue, recv_buffer);

            for (int i = 0; i < recv_queue->event_count; ++i) {

                NetEvent *event = recv_queue->events[i];
                void *args = event->args;

                // Run game logic + rendering based on NetEvents HERE
                switch (event->protocol) {

                case PERIODIC_HANDSHAKE: {
                    NetArgs_PeriodicHandshake *nargs = args;
                    printf("we GOT from server: %d\n", nargs->id);
                    break;
                }

                case CLIENT_LIST: {
                    ClientList *nargs = args;
                    client_id = nargs->local_client_id;
                    printf("Our client ID: %d\n", client_id);
                    print_client_list(nargs->info_list);
                    break;
                }

                default:
                    break;
                }
            }
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

        for (int i = 0; i < 2; ++i) {
            NetArgs_PeriodicHandshake *test_args = malloc(sizeof(NetArgs_PeriodicHandshake));
            test_args->id = rand();

            printf("rand: %d\n", test_args->id);
            NetEvent *test_event = net_event_new(PERIODIC_HANDSHAKE, test_args);

            insert_event(send_queue, test_event);
        }

        // Finally, send event queue
        send_event_queue(send_queue, to_server);
        empty_net_event_queue(send_queue);

        usleep(TICK_TIME_MICROSECONDS);
    }
}
