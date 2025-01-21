#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>

//Card struct for uno cards
typedef struct card card;
struct card{
  int color;
  int num;
  SDL_Rect rect;
  SDL_Rect textRect;
};
//Used for shared memory to record gamestate
typedef struct gameState gameState;
struct gameState{
  int client_id;
  card lastCard;
  int currentUno;
};
//Makes a new random card
card generate_card();
//Makes a deck of cards
void generate_cards(card * cards, int num, int width, int height);
//Removes a card and writes it to shared memory
void play_card(card * cards, card played, int num_card);
//Adds a card and sets width and height
card add_card(card * deck,int num, int width, int height);

#endif