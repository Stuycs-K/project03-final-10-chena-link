#ifndef SDL_H
#define SDL_H

#include <SDL2/SDL.h>

//Client render
void render(SDL_Renderer * renderer, SDL_Texture** textures, card * deck,int num);
void SDLInit();
void SDLInitText(SDL_Texture ** textures, SDL_Renderer * renderer);
#endif