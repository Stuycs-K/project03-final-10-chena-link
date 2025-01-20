#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "../client/baseclient.h"

#ifndef GSERVERWAITUI_H
#define GSERVERWAITUI_H

void renderGServerWait(SDL_Renderer *renderer, GServerInfo *serverInfo, BaseClient *gclient);

#endif