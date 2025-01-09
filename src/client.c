#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "client.h"
#include "game.h"
#include "network/pipehandshake.h"
#include "network/pipenet.h"
#include "network/pipenetevents.h"

void client_main(void) {
    srand(getpid());
    net_init();

    NetEvent *handshake_event = create_handshake_event();

    int to_server, from_server;

    to_server = client_setup("TEMP", handshake_event);
    from_server = client_handshake(to_server, handshake_event);

    if (from_server == -1) {
        printf("Connection failed\n");
        return;
    }

    NetEventQueue *net_send_queue = net_event_queue_new();

    card deck[100];
    int num_cards = 7;
    generate_cards(deck,num_cards);
    char input[10];
    while (1) {
        empty_net_event_queue(net_send_queue);

        /*for (int i = 0; i < 2; ++i) {
            NetArgs_PeriodicHandshake *test_args = malloc(sizeof(NetArgs_PeriodicHandshake));
            test_args->id = rand();

            printf("rand: %d\n", test_args->id);
            NetEvent *test_event = net_event_new(PERIODIC_HANDSHAKE, test_args);

            insert_event(net_send_queue, test_event);
        }

        send_event_queue(net_send_queue, to_server);
        */
        for(int i = 0; i < num_cards;i++){
            printf("%d: color: %d num: %d\n",i,deck[i].color,deck[i].num);
        }
        fgets(input, sizeof(input), stdin);
        if(input[0] == 'l'){
            deck[num_cards] = generate_card();
            num_cards ++;
        }
        if(input[0] == 'p'){
            //Card from shared memory
            card picked;
            picked.num = 1;
            picked.color = 1;
            //If num or color from picked is the same as shared memory then play_card
            //else pick new card or draw
            play_card(deck,picked,num_cards);
            num_cards--;
        }
        usleep(1000000);
    }
}
