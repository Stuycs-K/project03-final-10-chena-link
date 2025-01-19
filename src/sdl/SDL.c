#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <unistd.h>

#include "../game.h"

void render(SDL_Renderer * renderer, SDL_Texture** textures, card * deck,int num){
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
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
        SDL_Rect textRect = deck[i].rect;
        textRect.x += textRect.w/2;
        textRect.y += textRect.h/2;
        SDL_RenderCopy(renderer, textures[deck[i].num], NULL, &textRect);
    }
    SDL_RenderPresent(renderer);
}
void SDLInit(){
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL_Error: %s\n", SDL_GetError());
        return;
    }
    if (TTF_Init() == -1) {
        printf("Failed to initialize SDL_ttf: %s\n", TTF_GetError());
        SDL_Quit();
        return;
    }
}
void SDLInitText(SDL_Texture ** textures, SDL_Renderer * renderer){
    TTF_Font * font = TTF_OpenFont("src/sdl/OpenSans-Regular.ttf", 24);
    if (font == NULL) {
        printf("Failed to load font: %s\n", TTF_GetError());
        return;
    }
    SDL_Surface * surface;
    SDL_Color color = {255,255,255,255};
    char buffer[2];
    for(int i = 0; i < 10; i++){
        sprintf(buffer,"%d",i);
        surface = TTF_RenderText_Solid(font,buffer,color);
        if (surface == NULL) {
            printf("Failed to create surface for text: %s\n", TTF_GetError());
            return;
        }
        textures[i] = SDL_CreateTextureFromSurface(renderer,surface);
        SDL_FreeSurface(surface);
    }
}