#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <unistd.h>

#include "../client/baseclient.h"
#include "../game.h"
#include "../shared.h"
#include "SDL.h"
#include "sdlutil.h"

SDL_Rect draw = {240, HEIGHT / 2 - HEIGHT / 16, WIDTH / 15, HEIGHT / 8};
SDL_Rect statecard = {WIDTH / 2 - WIDTH / 30, HEIGHT / 2 - HEIGHT / 16, WIDTH / 15, HEIGHT / 8};
SDL_Rect statenum = {WIDTH / 2 - WIDTH / 120, HEIGHT / 2 - HEIGHT / 64, WIDTH / 60, HEIGHT / 32};

SDL_Rect first = {40, HEIGHT / 2 - HEIGHT / 16, WIDTH / 16, HEIGHT / 8};
SDL_Rect second = {WIDTH / 2 - WIDTH / 32, 40, WIDTH / 16, HEIGHT / 8};
SDL_Rect third = {WIDTH - 40 - WIDTH / 16, HEIGHT / 2 - HEIGHT / 16, WIDTH / 16, HEIGHT / 8};

SDL_Rect leaveButton = {WIDTH - 125, 20, 120, 40};

// Now this is ugly
SDL_Rect otherPlayerRectList[3] = {
    {40, HEIGHT / 2 - HEIGHT / 16, WIDTH / 16, HEIGHT / 8},                       // First (left)
    {WIDTH / 2 - WIDTH / 32, 40, WIDTH / 16, HEIGHT / 8},                         // Second (above)
    {WIDTH - 40 - WIDTH / 16, HEIGHT / 2 - HEIGHT / 16, WIDTH / 16, HEIGHT / 8}}; // Third (right)

SDL_Rect Uno = {3 * WIDTH / 4, 3 * HEIGHT / 4, WIDTH / 6, HEIGHT / 8};

void render(SDL_Renderer *renderer, SDL_Texture **textures, card *deck, int num, card state, int *others, int client_id, int uno, BaseClient *gclient) {
    modCoords(deck, num);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    renderBackground(renderer, textures, state, others, client_id, gclient);

    SDL_SetRenderDrawColor(renderer, RED, 255);
    SDL_RenderFillRect(renderer, &leaveButton);
    SDL_Point disconnectTextPosition = {leaveButton.x + leaveButton.w / 2, leaveButton.y + leaveButton.h / 2};
    renderTextLabel(renderer, "Disconnect", &disconnectTextPosition, X_CENTER | Y_CENTER, NULL, NULL);

    for (int i = 0; i < num; i++) {
        if (deck[i].color == 0) {
            SDL_SetRenderDrawColor(renderer, RED, 255);
        }
        if (deck[i].color == 1) {
            SDL_SetRenderDrawColor(renderer, BLUE, 255);
        }
        if (deck[i].color == 2) {
            SDL_SetRenderDrawColor(renderer, GREEN, 255);
        }
        if (deck[i].color == 3) {
            SDL_SetRenderDrawColor(renderer, YELLOW, 255);
        }
        SDL_RenderFillRect(renderer, &deck[i].rect);
        SDL_RenderCopy(renderer, textures[deck[i].num], NULL, &deck[i].textRect);
    }
    if (uno == 1) {
        SDL_Texture *unoIMG = IMG_LoadTexture(renderer, "src/sdl/uno.png");
        if (unoIMG == NULL) {
            printf("Unable to load texture! SDL_image Error: %s\n", IMG_GetError());
            return;
        }
        SDL_RenderCopy(renderer, unoIMG, NULL, &Uno);
    }
    SDL_RenderPresent(renderer);
}

void SDLInit() {
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

void SDLInitText(SDL_Texture **textures, SDL_Renderer *renderer) {
    TTF_Font *font = TTF_OpenFont("OpenSans-Regular.ttf", 18);
    if (font == NULL) {
        printf("Failed to load font: %s\n", TTF_GetError());
        return;
    }
    SDL_Surface *surface;
    SDL_Color color = {0, 0, 0, 255};
    char buffer[2];
    for (int i = 0; i < 100; i++) {
        sprintf(buffer, "%d", i);
        surface = TTF_RenderText_Solid(font, buffer, color);
        if (surface == NULL) {
            printf("Failed to create surface for text: %s\n", TTF_GetError());
            return;
        }
        textures[i] = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
    }
    TTF_CloseFont(font);
}

int EventPoll(SDL_Event event, card *deck, int num) {
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_MOUSEBUTTONUP:
            printf("Mouse button %d released at (%d, %d)\n", event.button.button, event.button.x, event.button.y);
            if (event.button.x > 240 && event.button.x < 240 + WIDTH / 15 && event.button.y > HEIGHT / 2 - HEIGHT / 16 && event.button.y < HEIGHT / 2 - HEIGHT / 16 + HEIGHT / 8) {
                return -2;
            }
            if (event.button.x > 3 * WIDTH / 4 && event.button.x < 3 * WIDTH / 4 + WIDTH / 6 && event.button.y > 3 * HEIGHT / 4 && event.button.y < 3 * HEIGHT / 4 + HEIGHT / 8) {
                return -4;
            }
            for (int i = 0; i < num; i++) {
                if (event.button.x > deck[i].rect.x && event.button.x < deck[i].rect.x + deck[i].rect.w && event.button.y > deck[i].rect.y && event.button.y < deck[i].rect.y + deck[i].rect.h) {
                    printf("%d\n", i);
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

void modCoords(card *deck, int num) {
    int increment = (WIDTH - deck[0].rect.w * num) / 2 - deck[0].rect.x;
    if (increment != 0) {
        for (int i = 0; i < num; i++) {
            deck[i].rect.x += increment;
            deck[i].textRect.x += increment;
        }
    }
}

void renderBackground(SDL_Renderer *renderer, SDL_Texture **textures, card state, int *others, int client_id, BaseClient *gclient) {
    if (state.color == 0) {
        SDL_SetRenderDrawColor(renderer, RED, 255);
    }
    if (state.color == 1) {
        SDL_SetRenderDrawColor(renderer, BLUE, 255);
    }
    if (state.color == 2) {
        SDL_SetRenderDrawColor(renderer, GREEN, 255);
    }
    if (state.color == 3) {
        SDL_SetRenderDrawColor(renderer, YELLOW, 255);
    }
    SDL_RenderFillRect(renderer, &statecard);
    SDL_RenderCopy(renderer, textures[state.num], NULL, &statenum);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &draw);
    TTF_Font *font = TTF_OpenFont("OpenSans-Regular.ttf", 24);
    if (!font) {
        printf("Error loading font: %s\n", TTF_GetError());
        return;
    }
    ClientInfoNode *node = gclient->client_info_list;
    int counter = 0;
    int clients[4];
    char *name[4];
    while (node != NULL) {
        clients[counter] = node->id;
        name[counter] = node->name;
        node = node->next;
        counter++;
    }
    SDL_Surface *surface;
    SDL_Texture *textTexture;
    SDL_Color color = {255, 255, 255, 255};
    SDL_Rect textRect;
    for (int i = 0; i < 4; i++) {
        if (others[i * 2] == client_id) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            for (int offset = 1; offset <= 3; offset++) {
                int a = (i - offset + 4) % 4;
                if (offset == 1) {
                    for (int i = 0; i < counter; i++) {
                        if (clients[i] == others[a * 2]) {
                            surface = TTF_RenderText_Solid(font, name[i], color);
                            textTexture = SDL_CreateTextureFromSurface(renderer, surface);
                            textRect = first;
                            textRect.y -= HEIGHT / 8;
                            textRect.w *= 1.5;
                            SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
                            SDL_FreeSurface(surface);
                            SDL_DestroyTexture(textTexture);
                        }
                    }
                    SDL_RenderFillRect(renderer, &first);
                    SDL_RenderCopy(renderer, textures[others[a * 2 + 1]], NULL, &first);
                } else if (offset == 2) {
                    for (int i = 0; i < counter; i++) {
                        if (clients[i] == others[a * 2]) {
                            surface = TTF_RenderText_Solid(font, name[i], color);
                            textTexture = SDL_CreateTextureFromSurface(renderer, surface);
                            textRect = second;
                            textRect.y += HEIGHT / 8;
                            textRect.w *= 1.5;
                            SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
                            SDL_FreeSurface(surface);
                            SDL_DestroyTexture(textTexture);
                        }
                    }
                    SDL_RenderFillRect(renderer, &second);
                    SDL_RenderCopy(renderer, textures[others[a * 2 + 1]], NULL, &second);
                } else if (offset == 3) {
                    for (int i = 0; i < counter; i++) {
                        if (clients[i] == others[a * 2]) {
                            surface = TTF_RenderText_Solid(font, name[i], color);
                            textTexture = SDL_CreateTextureFromSurface(renderer, surface);
                            textRect = third;
                            textRect.y -= HEIGHT / 8;
                            textRect.w *= 1.5;
                            textRect.x -= WIDTH / 32;
                            SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
                            SDL_FreeSurface(surface);
                            SDL_DestroyTexture(textTexture);
                        }
                    }
                    SDL_RenderFillRect(renderer, &third);
                    SDL_RenderCopy(renderer, textures[others[a * 2 + 1]], NULL, &third);
                }
            }
        }
    }
}