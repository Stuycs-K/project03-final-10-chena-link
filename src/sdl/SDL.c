#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <unistd.h>

#include "../game.h"
#include "SDL.h"

#define width 800
#define height 800
SDL_Rect draw = {240,430,width/15,height/8};
SDL_Rect statecard = {380,430,width/15,height/8};
SDL_Rect statenum = {380+width/30-width/120,430+height/16-height/64,width/60,height/32};

void render(SDL_Renderer * renderer, SDL_Texture** textures, card * deck,int num, card state){
    modCoords(deck,num);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    renderBackground(renderer,textures,state);
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
        SDL_RenderCopy(renderer, textures[deck[i].num], NULL, &deck[i].textRect);
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
    TTF_Font * font = TTF_OpenFont("src/sdl/OpenSans-Regular.ttf", 18);
    if (font == NULL) {
        printf("Failed to load font: %s\n", TTF_GetError());
        return;
    }
    SDL_Surface * surface;
    SDL_Color color = {0,0,0,255};
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
    TTF_CloseFont(font);
}
int EventPoll(SDL_Event event, card * deck, int num){
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_MOUSEBUTTONUP:
                printf("Mouse button %d released at (%d, %d)\n",event.button.button,event.button.x,event.button.y);
                if(event.button.x > 240 && event.button.x < 240+40 && event.button.y > 430 && event.button.y < 430+60){
                    return -2;
                }
                for(int i = 0; i < num; i ++){
                    if(event.button.x > deck[i].rect.x && event.button.x < deck[i].rect.x+deck[i].rect.w && event.button.y > deck[i].rect.y && event.button.y < deck[i].rect.y + deck[i].rect.h){
                        printf("%d\n",i);
                        return i;
                    }
                }
                break;
            case SDL_QUIT:
                return -3;
            default:
                break;
        }
    }
    return -1;
}
void modCoords(card * deck,int num){
    int increment = (width - deck[0].rect.w * num)/2 - deck[0].rect.x;
    if(increment != 0){
        for(int i = 0; i < num; i ++){
            deck[i].rect.x += increment;
            deck[i].textRect.x += increment;
        }
    }
}
void renderBackground(SDL_Renderer * renderer,SDL_Texture** textures,card state, int *others){
    if(state.color == 0){
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    }
    if(state.color == 1){
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
    }
    if(state.color == 2){
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    }
    if(state.color == 3){
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
    }
    SDL_RenderFillRect(renderer, &statecard);
    SDL_RenderCopy(renderer, textures[state.num], NULL, &statenum);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &draw);
}