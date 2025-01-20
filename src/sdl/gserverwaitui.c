#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <unistd.h>

#include "../client/baseclient.h"
#include "../shared.h"
#include "sdlutil.h"

int titleLabelFontSize = 20;
int normalFontSize = 16;

void drawServerInfo(SDL_Renderer *renderer, GServerInfo *serverInfo, BaseClient *gclient) {
    SDL_Point buttonPosition = {5, 5};
    renderTextLabel(renderer, serverInfo->name, &buttonPosition, X_LEFT | Y_TOP, NULL, &titleLabelFontSize);

    char playerStatus[32];
    snprintf(playerStatus, sizeof(playerStatus), "%d / %d", serverInfo->current_clients, serverInfo->max_clients);

    SDL_Point playerStatusPosition = {5, 30};
    renderTextLabel(renderer, playerStatus, &playerStatusPosition, X_LEFT | Y_TOP, NULL, &normalFontSize);
}

void drawDisconnectButton(SDL_Renderer *renderer, GServerInfo *serverInfo, BaseClient *gclient) {
    client_disconnect_from_server(gclient);
}

void drawHostControls(SDL_Renderer *renderer, GServerInfo *serverInfo, BaseClient *gclient) {
    if (serverInfo->host_id != gclient->client_id) {
        printf("the host is %d but we're %d\n", serverInfo->host_id, gclient->client_id);
        return;
    }
}

void drawClientList(SDL_Renderer *renderer, GServerInfo *serverInfo, BaseClient *gclient) {
    const int startX = 40;
    const int startY = 100;
    const int sizeX = WIDTH - 2 * startX;
    const int sizeY = 70;
    const int yPadding = 5;
    int currentY = startY;

    ClientInfoNode *node = gclient->client_info_list;
    while (node != NULL) {
        int id = node->id;

        SDL_Rect mainPanel;
        mainPanel.w = sizeX;
        mainPanel.h = sizeY;

        mainPanel.x = startX;
        mainPanel.y = currentY;

        currentY += (sizeY + yPadding);

        SDL_SetRenderDrawColor(renderer, RED, 255);
        SDL_RenderFillRect(renderer, &mainPanel);

        SDL_Point textPosition = {mainPanel.x, mainPanel.y};
        renderTextLabel(renderer, node->name, &textPosition, X_LEFT | Y_TOP, NULL, &normalFontSize);

        node = node->next;
    }
}

void renderGServerWait(SDL_Renderer *renderer, GServerInfo *serverInfo, BaseClient *gclient) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    drawServerInfo(renderer, serverInfo, gclient);
    drawClientList(renderer, serverInfo, gclient);
    drawHostControls(renderer, serverInfo, gclient);

    SDL_RenderPresent(renderer);
}

int handleGServerWaitEvent() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_MOUSEBUTTONDOWN:
            SDL_Point clickPosition;
            clickPosition.x = event.button.x;
            clickPosition.y = event.button.y;

            break;
        default:
            break;
        }
    }

    return 0;
}