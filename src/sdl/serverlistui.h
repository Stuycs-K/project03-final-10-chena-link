#ifndef SERVERLISTUI_H
#define SERVERLISTUI_H

void renderServerList(SDL_Renderer *renderer, GServerInfoList *serverList);

int handleServerListEvent(SDL_Event event);

#endif