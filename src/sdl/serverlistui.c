#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <unistd.h>

#include "../client/baseclient.h"
#include "../shared.h"
#include "SDL.h"
#include "sdlutil.h"
#include "serverlistui.h"

const int startX = 40;
const int startY = 40;

const int sizeX = WIDTH - 2 * startX;
const int sizeY = 60;

const int yPadding = 4;

int serverNameFontSize = 15;

typedef struct GServerInfoPanel GServerInfoPanel;
struct GServerInfoPanel {
    SDL_Rect mainPanel;
    SDL_Rect joinButton;
    SDL_bool isEnabled;
};

GServerInfoPanel gserverPanels[MAX_CSERVER_GSERVERS];

void renderServerList(SDL_Renderer *renderer, GServerInfoList *serverList) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    for (int i = 0; i < MAX_CSERVER_GSERVERS; ++i) {
        gserverPanels[i].isEnabled = SDL_FALSE;
    }
    printf("to dust i guess\n");

    int currentY = startY;
    for (int i = 0; i < MAX_CSERVER_GSERVERS; ++i) {
        GServerInfo *currentInfo = serverList[i];

        if (currentInfo->status == GSS_UNRESERVED || currentInfo->status == GSS_SHUTTING_DOWN) {
            continue;
        }

        SDL_Rect mainPanel;
        mainPanel.w = sizeX;
        mainPanel.h = sizeY;

        mainPanel.x = startX;
        mainPanel.y = currentY;

        currentY += (sizeY + yPadding);

        SDL_Rect joinButton;
        joinButton.w = mainPanel.w / 8;
        joinButton.h = 0.8 * mainPanel.h;

        joinButton.x = startX + 400;
        joinButton.y = mainPanel.h + mainPanel.y / 2; // Centered

        GServerInfoPanel panel = gserverPanels[i];
        panel.mainPanel = mainPanel;
        panel.joinButton = joinButton;
        panel.isEnabled = SDL_TRUE;
    }

    for (int i = 0; i < MAX_CSERVER_GSERVERS; ++i) {
        GServerInfoPanel panel;
        GServerInfo *currentInfo = serverList[i];

        if (panel.isEnabled == SDL_FALSE) { // Don't render
            continue;
        }

        // First, the main panel
        SDL_SetRenderDrawColor(renderer, RED, 255);
        SDL_RenderFillRect(renderer, &panel.mainPanel);

        // Server name
        SDL_Point namePosition = {panel.mainPanel.x, panel.mainPanel.y};
        renderTextLabel(renderer, currentInfo->name, &namePosition, X_LEFT | Y_TOP, NULL, &serverNameFontSize);

        // Player status
        char playerStatus[32];
        snprintf(playerStatus, sizeof(playerStatus), "%d / %d", currentInfo->current_clients, currentInfo->max_clients);

        SDL_Point playersPosition = {panel.mainPanel.x, panel.mainPanel.y};
        renderTextLabel(renderer, playerStatus, &playersPosition, X_RIGHT | Y_BOTTOM, NULL, &serverNameFontSize);

        SDL_SetRenderDrawColor(renderer, GREEN, 255);
        SDL_RenderFillRect(renderer, &panel.joinButton);

        SDL_Point joinLabelPosition = {panel.joinButton.x, panel.joinButton.y};
        renderTextLabel(renderer, "Join", &joinLabelPosition, X_CENTER | Y_CENTER, NULL, &serverNameFontSize);
    }

    SDL_RenderPresent(renderer);
}

int handleServerListEvent(SDL_Event event) {
    switch (event.type) {
    case SDL_MOUSEBUTTONDOWN:
        break;
    default:
        break;
    }
}