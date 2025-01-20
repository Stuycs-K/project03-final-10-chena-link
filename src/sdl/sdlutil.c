#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <unistd.h>

#include "../shared.h"
#include "sdlutil.h"

void renderTextLabel(SDL_Renderer *renderer, char *text, SDL_Point *position, int positionRelativeTo, SDL_Color *color, int *fontSize) {
    TTF_Font *font = TTF_OpenFont("OpenSans-Regular.ttf", (fontSize == NULL ? 18 : *fontSize));
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

    SDL_Rect destinationRect;
    destinationRect.w = textSurface->w;
    destinationRect.h = textSurface->h;

    if (positionRelativeTo & X_LEFT) {
        destinationRect.x = position->x;
    } else if (positionRelativeTo & X_CENTER) {
        destinationRect.x = position->x - destinationRect.w / 2;
    } else if (positionRelativeTo & X_RIGHT) {
        destinationRect.x = position->x - destinationRect.w;
    }

    if (positionRelativeTo & Y_TOP) {
        destinationRect.y = position->y;
    } else if (positionRelativeTo & Y_CENTER) {
        destinationRect.y = position->y - destinationRect.h / 2;
    } else if (positionRelativeTo & Y_BOTTOM) {
        destinationRect.y = position->y - destinationRect.h;
    }

    SDL_RenderCopy(renderer, texture, NULL, &destinationRect);

    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(texture);

    TTF_CloseFont(font);
}