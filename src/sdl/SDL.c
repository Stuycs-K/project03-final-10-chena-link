#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <unistd.h>

#include "../client/baseclient.h"
#include "../game.h"
#include "SDL.h"

#define RED 215, 38, 0
#define BLUE 9, 86, 191
#define GREEN 55, 151, 17
#define YELLOW 236, 212, 7
#define WHITE 255, 255, 255

#define width 800
#define height 800

SDL_Rect draw = {240, height / 2 - height / 16, width / 15, height / 8};
SDL_Rect statecard = {width / 2 - width / 30, height / 2 - height / 16, width / 15, height / 8};
SDL_Rect statenum = {width / 2 - width / 120, height / 2 - height / 64, width / 60, height / 32};

SDL_Rect first = {40, height / 2 - height / 16, width / 16, height / 8};
SDL_Rect second = {width / 2 - width / 32, 40, width / 16, height / 8};
SDL_Rect third = {width - 40 - width / 16, height / 2 - height / 16, width / 16, height / 8};

// Now this is ugly
SDL_Rect otherPlayerRectList[3] = {
    {40, height / 2 - height / 16, width / 16, height / 8},                       // First (left)
    {width / 2 - width / 32, 40, width / 16, height / 8},                         // Second (above)
    {width - 40 - width / 16, height / 2 - height / 16, width / 16, height / 8}}; // Third (right)

SDL_Rect Uno = {3 * width / 4, 3 * height / 4, width / 6, height / 8};

// Util
void renderTextLabel(SDL_Renderer *renderer, char *text, SDL_Rect *rect, SDL_Color *color) {
    TTF_Font *font = TTF_OpenFont("OpenSans-Regular.ttf", 18);
    if (!font) {
        printf("Error loading font: %s\n", TTF_GetError());
        return;
    }

    SDL_Surface *textSurface = NULL;

    if (color == NULL) { // Default color is white
        SDL_Color textColor = {WHITE};
        textSurface = TTF_RenderText_Solid(font, text, textColor);
    } else {
        textSurface = TTF_RenderText_Solid(font, text, *color);
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, textSurface);

    SDL_RenderCopy(renderer, texture, NULL, rect);

    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(texture);

    TTF_CloseFont(font);
}

void renderPlayerNameLabel(SDL_Renderer *renderer, char *name, int xCenter, int yCenter, SDL_Color *color) {
    int w = 120;
    int h = 35;

    int x = xCenter - w / 2;
    int y = yCenter - h / 2;

    SDL_Rect nameRect = {x, y, w, h};
    renderTextLabel(renderer, name, &nameRect, NULL);
}

void render(SDL_Renderer *renderer, SDL_Texture **textures, card *deck, int num, card state, int *others, int client_id, int uno, BaseClient *gclient) {
    modCoords(deck, num);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    renderBackground(renderer, textures, state, others, client_id, gclient);
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
            if (event.button.x > 240 && event.button.x < 240 + width / 15 && event.button.y > height / 2 - height / 16 && event.button.y < height / 2 - height / 16 + height / 8) {
                return -2;
            }
            if (event.button.x > 3 * width / 4 && event.button.x < 3 * width / 4 + width / 6 && event.button.y > 3 * height / 4 && event.button.y < 3 * height / 4 + height / 8) {
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
    int increment = (width - deck[0].rect.w * num) / 2 - deck[0].rect.x;
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
    TTF_Font *font = TTF_OpenFont("OpenSans-Regular.ttf", 18);
    if (!font) {
        printf("Error loading font: %s\n", TTF_GetError());
        return;
    }

    renderPlayerNameLabel(renderer, gclient->name, width * 0.5, 0.8 * height, NULL);
    /*
    ClientInfoNode *node = gclient->client_info_list;
    while (node != NULL) {
        int current_client_id = node->id;

        for (int i = 0; i < 4; ++i) {
            if (others[i * 2] != current_client_id) {
                continue;
            }

            printf("Rendering username: %s\n", node->name);

            SDL_Color color = {RED};

            renderTextLabel(renderer, node->name, NULL, &color);

            SDL_Surface *surface = TTF_RenderText_Solid(font, node->name, color);
            SDL_Texture *username = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_FreeSurface(surface);
            SDL_RenderCopy(renderer, username, NULL, &first);

            break;
        }

        node = node->next;
    }
    */

    SDL_Surface *surface;
    SDL_Color color = {255, 255, 255, 255};
    for (int i = 0; i < 4; i++) {
        if (others[i * 2] == client_id) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

            for (int offset = 1; offset <= 3; offset++) {
                int a = (i - offset + 4) % 4;

                SDL_RenderFillRect(renderer, &otherPlayerRectList[offset - 1]);
                SDL_RenderCopy(renderer, textures[others[a * 2 + 1]], NULL, &otherPlayerRectList[offset - 1]);
            }
        }
    }
}