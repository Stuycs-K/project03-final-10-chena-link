#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "game.h"

//Generates 1 card
card generate_card(){
  card drawn;
  drawn.color = rand()%4;
  drawn.num = rand()%10;
  return drawn;
}

//Generates num cards
void generate_cards(card * cards, int num, int width, int height){
  for(int i = 0; i < num; i ++){
    cards[i] = generate_card();
    cards[i].rect.x = width/15 * i;
    cards[i].rect.y = 0;
    cards[i].rect.w = width/15;
    cards[i].rect.h = width/8;
  }
}

//Plays a card
//Probably going to look for a card in the array until the information matches
void play_card(card * cards, card played, int num_card){
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
