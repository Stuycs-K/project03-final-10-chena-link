#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>

typedef struct card card;
struct card{
  int color;
  int num;
  SDL_Rect rect;
};
typedef struct gameState gameState;
struct gameState{
  int client_id;
  card lastCard;
};
//Makes a new random card
card generate_card();
//Makes a deck of cards
void generate_cards(card * cards, int num);
//Removes a card and writes it to shared memory
void play_card(card * cards, card played, int num_card);

#endif