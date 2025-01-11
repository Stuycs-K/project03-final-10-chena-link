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

// May as well be globals
int cclient_id;
int to_cserver;
int from_cserver;
NetEventQueue *cserver_send_queue;
NetEventQueue *cserver_recv_queue;

void connect_to_cserver() {
    cserver_send_queue = net_event_queue_new();
    cserver_recv_queue = net_event_queue_new();

    NetEvent *handshake_event = create_handshake_event();
    NetArgs_Handshake *handshake = handshake_event->args;

    client_setup(CSERVER_WKP_NAME, handshake_event);
    int succeeded = client_handshake(handshake_event);

    if (succeeded == -1) {
        printf("Connection to CServer failed\n");
        return;
    }

    cclient_id = -1;
    to_cserver = handshake->client_to_server_fd;
    from_cserver = handshake->server_to_client_fd;

    free_handshake_event(handshake_event);
}

void recv_from_cserver() {
    void *recv_buffer;
    while (recv_buffer = read_into_buffer(from_cserver)) {
        recv_event_queue(cserver_recv_queue, recv_buffer);

        for (int i = 0; i < cserver_recv_queue->event_count; ++i) {
            NetEvent *event = cserver_recv_queue->events[i];
            void *args = event->args;

            switch (event->protocol) {

            case PERIODIC_HANDSHAKE: {
                NetArgs_PeriodicHandshake *nargs = args;
                printf("we GOT from server: %d\n", nargs->id);
                break;
            }

            case CLIENT_LIST: {
                ClientList *nargs = args;
                cclient_id = nargs->local_client_id;
                printf("Our CServer client ID: %d\n", cclient_id);
                print_client_list(nargs->info_list);
                break;
            }

            default:
                break;
            }
        }
    }
}

NetEvent *connect_to_gserver() {
    NetEvent *handshake_event = create_handshake_event();
    NetArgs_Handshake *handshake = handshake_event->args;

    client_setup("G69", handshake_event);
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
    gserver_client_list_event->cleanup_behavior = NEVENT_PERSISTENT;

    // Everything below here will be looped in the future!
    NetEvent *handshake_event = connect_to_gserver();
    if (handshake_event == NULL) {
        return;
    }
    NetArgs_Handshake *handshake = handshake_event->args;

    int gclient_id = handshake->client_id;
    int to_server = handshake->client_to_server_fd;
    int from_server = handshake->server_to_client_fd;
    // SET RECEIVE FO TO NONBLOCK MODE!
    set_nonblock(from_server);

    printf("Sending to %d\n", to_server);

    NetEventQueue *send_queue = net_event_queue_new();
    NetEventQueue *recv_queue = net_event_queue_new();

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
                    gclient_id = nargs->local_client_id;
                    printf("Our GServer client ID: %d\n", gclient_id);
                    print_client_list(nargs->info_list);
                    break;
                }

                default:
                    break;
                }
            }
        }

        if (gclient_id < 0) {
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

            insert_event(send_queue, test_event);
        }

        // Finally, send event queue
        send_event_queue(send_queue, to_server);
        empty_net_event_queue(send_queue);

        usleep(TICK_TIME_MICROSECONDS);
    }
}
