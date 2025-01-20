#ifndef SDL_H
#define SDL_H

#include <SDL2/SDL.h>
#include "../game.h"
#include "../client/baseclient.h"

//Client render
void render(SDL_Renderer *renderer, SDL_Texture **textures, card *deck, int num, gameState * state, int *others, int client_id, int uno, BaseClient *gclient);
//Initializes SDL and TTF
void SDLInit();
//Creates textures for numbers 0-9
void SDLInitText(SDL_Texture ** textures, SDL_Renderer * renderer);
//Checks for mouseclick and returns the type of action
int EventPoll(SDL_Event event, card * deck, int num);
//Centers the cards coordinates to look better
void modCoords(card * deck,int num);
//Renders the background cards
void renderBackground(SDL_Renderer * renderer,SDL_Texture** textures,gameState * state, int *others,int client_id,BaseClient*gclient);
#endif