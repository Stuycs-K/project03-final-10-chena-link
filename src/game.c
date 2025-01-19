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
    cards[i].rect.w = width/15;
    cards[i].rect.h = height/8;
    cards[i].rect.y = height-cards[i].rect.h;
    cards[i].textRect = cards[i].rect;
    cards[i].textRect.x += cards[i].textRect.w/2 - cards[i].rect.w/8;
    cards[i].textRect.y += cards[i].textRect.h/2 - cards[i].rect.h/8;
    cards[i].textRect.w = cards[i].rect.w/4;
    cards[i].textRect.h = cards[i].rect.h/4;
  }
}

card add_card(card * deck,int num, int width, int height){
  card Card = generate_card();
  Card.rect.x = deck[num-1].rect.x + deck[num-1].rect.w;
  Card.rect.y = deck[num-1].rect.y;
  Card.rect.w = deck[num-1].rect.w;
  Card.rect.h = deck[num-1].rect.h;
  Card.textRect = Card.rect;
  Card.textRect.x += Card.textRect.w/2 - Card.rect.w/8;
  Card.textRect.y += Card.textRect.h/2 - Card.rect.h/8;
  Card.textRect.w = Card.rect.w/4;
  Card.textRect.h = Card.rect.h/4;
  return Card;
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
