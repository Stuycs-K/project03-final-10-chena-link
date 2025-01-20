#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <unistd.h>

#ifndef SDLUTIL_H
#define SDLUTIL_H

#define X_LEFT 0x1
#define X_CENTER 0x10
#define X_RIGHT 0x100
#define Y_TOP 0x1000
#define Y_CENTER 0x10000
#define Y_BOTTOM 0x100000

void renderTextLabel(SDL_Renderer *renderer, char *text, SDL_Point *position, int positionRelativeTo, SDL_Color *color, int *fontSize);

#endif