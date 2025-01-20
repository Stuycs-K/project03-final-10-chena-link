#ifndef SERVERLISTUI_H
#define SERVERLISTUI_H

#define SERVER_LIST_EVENT_RESERVE -1
#define SERVER_LIST_EVENT_NOTHING -2

void renderServerList(SDL_Renderer *renderer, GServerInfoList *serverList);

int handleServerListEvent();

#endif