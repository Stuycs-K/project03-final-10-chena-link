#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "../client/baseclient.h"

#ifndef GSERVERWAITUI_H
#define GSERVERWAITUI_H

#define GSERVER_WAITING_DISCONNECT -2
#define GSERVER_WAITING_START_GAME -1
#define GSERVER_WAITING_NOTHING 0

void renderGServerWait(SDL_Renderer *renderer, GServerInfo *serverInfo, BaseClient *gclient);

int handleGServerWaitEvent(GServerInfo *serverInfo, BaseClient *gclient);

#endif