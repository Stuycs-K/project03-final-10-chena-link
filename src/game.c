#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <time.h>

typedef struct card card;
struct card{
  int color;
  int num;
};

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
