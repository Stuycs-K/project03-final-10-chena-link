#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <time.h>

#include "gserver.h"
#include "pipehandshake.h"
#include "piperw.h"

//Makes a new subserver/client
GSubserver *gsubserver_new(int client_id) {
    GSubserver *gsubserver = malloc(sizeof(GSubserver));

    gsubserver->client_id = client_id;
    gsubserver->send_fd = -1;
    gsubserver->recv_fd = -1;
    gsubserver->pid = -1;

    return gsubserver;
}

// Finishes the server handshake
void gsubserver_init(GSubserver *gsubserver) {
    gsubserver->pid = getpid();

    int send_fd = server_handshake(gsubserver->recv_fd);

    // Handshake failed. Abort.
    if (send_fd == -1) {
        printf("Failed handshake\n");
        exit(EXIT_FAILURE);
    }

    printf("CONNECTION MADE WITH CLIENT!\n");

    while (1) {
        printf("server hello\n");
        usleep(1000000);
    }

    exit(EXIT_SUCCESS);
}

int gsubserver_is_inactive(GSubserver *gsubserver) {
    return (gsubserver->pid == -1 && gsubserver->recv_fd == -1);
}

char *get_client_to_server_fifo_name() {
    // return getpid();
    return "TEMP";
}

GServer *gserver_new() {
    GServer *gserver = malloc(sizeof(GServer));

    gserver->max_clients = 2;
    gserver->current_clients = 0;
    gserver->id = 0;
    gserver->name = NULL;

    // Populate with inactive subservers
    gserver->subservers = malloc(sizeof(GSubserver *) * gserver->max_clients);
    for (int i = 0; i < gserver->max_clients; ++i) {
        gserver->subservers[i] = gsubserver_new(i);
    }

    return gserver;
}

// -1 indicates the server is full
int gserver_get_free_client_id(GServer *gserver) {
    for (int i = 0; i < gserver->max_clients; ++i) {
        if (gsubserver_is_inactive(gserver->subservers[i])) {
            return i;
        }
    }

    return -1;
}

GSubserver *gserver_handle_connection(GServer *gserver, int recv_fd) {
    int client_id = gserver_get_free_client_id(gserver);

    // TODO: Send a kick message (this shouldn't be possible anyways)
    if (client_id == -1) {
        printf("Could not assign the client to a valid ID\n");
        return NULL;
    }

    GSubserver *chosen_subserver = gserver->subservers[client_id];
    chosen_subserver->recv_fd = recv_fd;

    gserver->current_clients++;

    return chosen_subserver;
}

static void handle_sigint(int signo) {
    if (signo != SIGINT) {
        return;
    }

    remove(get_client_to_server_fifo_name());
    exit(EXIT_SUCCESS);
}

void gserver_init(GServer *gserver) {
    signal(SIGINT, handle_sigint);

    int from_client;
    int to_client;

    while (1) {
        // TODO: Reverse the order and send connection failure messages
        if (gserver->current_clients >= gserver->max_clients) {
            continue;
        }
        from_client = server_setup(get_client_to_server_fifo_name());

        GSubserver *subserver = gserver_handle_connection(gserver, from_client);
        if (subserver == NULL) {
            continue;
        }

        pid_t pid = fork();

        if (pid == 0) {
            gsubserver_init(subserver);
        } else {
            gserver->subservers[subserver->client_id]->pid = pid;
        }
    }
}

//Generates 1 card
card generate_card(){
  //Write new len of array back to server
  srand(time(NULL));
  card drawn;
  drawn.color = rand()%4;
  drawn.num = rand()%10;
  return drawn;
}

//Generates num cards
card * generate_cards(card * cards, int num){
  card deck[num];
  for(int i = 0; i < num; i ++){
    deck[i] = generate_card();
  }
  return deck;
}

//Plays a card
//Probably going to look for a card in the array until the information matches
card * play_card(card * cards, card played){
  //Should be created with game server
  int shmid = shmget(getpid(),sizeof(card), IPC_CREAT | 0640);
  int i = 0;
  while (cards[i].num != -1){
    if(played.num == cards[i].num && played.color == cards[i].color){
      cards[i].num = -1;
      return cards;
    }
    i++;
  }
  return cards;
}
