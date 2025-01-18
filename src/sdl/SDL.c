#include <SDL2/SDL.h>
#include <stdio.h>
#include <unistd.h>

#include "../game.h"

void render(SDL_Renderer * renderer,card * deck,int num){
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    for(int i = 0; i < num; i ++){
        if(deck[i].color == 0){
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        }
        if(deck[i].color == 1){
            SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        }
        if(deck[i].color == 2){
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        }
        if(deck[i].color == 3){
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        }
        SDL_RenderFillRect(renderer, &deck[i].rect);
    }
    SDL_RenderPresent(renderer);
}