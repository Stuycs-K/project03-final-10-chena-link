#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>

#include "game.h"

//Generates 1 card
card generate_card(){
  //Write new len of array back to server
  card drawn;
  drawn.color = rand()%4;
  drawn.num = rand()%10;
  return drawn;
}

//Generates num cards
void generate_cards(card * cards, int num){
  for(int i = 0; i < num; i ++){
    cards[i] = generate_card();
  }
}

//Plays a card
//Probably going to look for a card in the array until the information matches
void play_card(card * cards, card played, int num_card){
  //Should be created with game server
  //int shmid = shmget(getpid(),sizeof(card), IPC_CREAT | 0640);
  for(int i = 0; i < num_card; i ++){
    if(played.num == cards[i].num && played.color == cards[i].color){
      cards[i].num = -1;
      for(int a = i; a < num_card-1;a++){
        cards[a].color = cards[a+1].color;
        cards[a].num = cards[a+1].num;
      }
      return;
    }
  }
}
