#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <unistd.h>

#include "../client/baseclient.h"
#include "../shared.h"
#include "sdlutil.h"
#include "serverlistui.h"

int serverNameFontSize = 15;
int serverListLabelFontSize = 20;

typedef struct GServerInfoPanel GServerInfoPanel;
struct GServerInfoPanel {
    SDL_Rect mainPanel;
    SDL_Rect joinButton;
    SDL_bool isEnabled;
};

GServerInfoPanel gserverPanels[MAX_CSERVER_GSERVERS];

SDL_Rect reserveButton = {WIDTH - 120, 5, 110, 40}; // Top right

void drawReserveButton(SDL_Renderer *renderer) {
    SDL_SetRenderDrawColor(renderer, BLUE, 255);
    SDL_RenderFillRect(renderer, &reserveButton);

    SDL_Point buttonPosition = {reserveButton.x, reserveButton.y};
    renderTextLabel(renderer, "CREATE LOBBY", &buttonPosition, X_LEFT | Y_TOP, NULL, &serverNameFontSize);
}

void drawServerListLabel(SDL_Renderer *renderer) {
    SDL_Point position = {0, 5};
    renderTextLabel(renderer, "Server List", &position, X_LEFT | Y_TOP, NULL, &serverListLabelFontSize);
}

void renderServerList(SDL_Renderer *renderer, GServerInfoList *serverList) {
    const int startX = 40;
    const int startY = 60;
    const int sizeX = WIDTH - 2 * startX;
    const int sizeY = 55;
    const int yPadding = 4;

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    for (int i = 0; i < MAX_CSERVER_GSERVERS; ++i) {
        gserverPanels[i].isEnabled = SDL_FALSE;
    }

    drawReserveButton(renderer);
    drawServerListLabel(renderer);

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

        joinButton.x = WIDTH - joinButton.w - 50;
        joinButton.y = mainPanel.y;

        gserverPanels[i].mainPanel = mainPanel;
        gserverPanels[i].joinButton = joinButton;
        gserverPanels[i].isEnabled = SDL_TRUE;
    }

    for (int i = 0; i < MAX_CSERVER_GSERVERS; ++i) {
        GServerInfo *currentInfo = serverList[i];

        if (gserverPanels[i].isEnabled == SDL_FALSE) { // Don't render
            continue;
        }

        // First, the main panel
        SDL_SetRenderDrawColor(renderer, RED, 255);
        SDL_RenderFillRect(renderer, &gserverPanels[i].mainPanel);

        // Server name
        SDL_Point namePosition = {gserverPanels[i].mainPanel.x, gserverPanels[i].mainPanel.y};
        renderTextLabel(renderer, currentInfo->name, &namePosition, X_LEFT | Y_TOP, NULL, &serverNameFontSize);

        // Player status
        char playerStatus[32];
        snprintf(playerStatus, sizeof(playerStatus), "%d / %d", currentInfo->current_clients, currentInfo->max_clients);

        SDL_Point playersPosition = {gserverPanels[i].mainPanel.x, gserverPanels[i].mainPanel.y};
        renderTextLabel(renderer, playerStatus, &playersPosition, X_RIGHT | Y_BOTTOM, NULL, &serverNameFontSize);

        SDL_SetRenderDrawColor(renderer, GREEN, 255);
        SDL_RenderFillRect(renderer, &gserverPanels[i].joinButton);

        SDL_Point joinLabelPosition = {gserverPanels[i].joinButton.x + gserverPanels[i].joinButton.w / 2, gserverPanels[i].joinButton.y + gserverPanels[i].joinButton.h / 2};
        renderTextLabel(renderer, "Join", &joinLabelPosition, X_CENTER | Y_CENTER, NULL, &serverNameFontSize);
    }

    SDL_RenderPresent(renderer);
}

int handleServerListEvent() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_MOUSEBUTTONDOWN:
            SDL_Point clickPosition;
            clickPosition.x = event.button.x;
            clickPosition.y = event.button.y;

            if (SDL_PointInRect(&clickPosition, &reserveButton)) {
                return SERVER_LIST_EVENT_RESERVE; // Reserve
            }

            for (int i = 0; i < MAX_CSERVER_GSERVERS; ++i) {
                GServerInfoPanel panel = gserverPanels[i];

                if (panel.isEnabled == SDL_FALSE) {
                    continue;
                }

                if (SDL_PointInRect(&clickPosition, &panel.joinButton)) {
                    printf("join %d\n", i);
                    return i; // We're joining this server
                }
            }
            break;
        default:
            break;
        }
    }

    return SERVER_LIST_EVENT_NOTHING;
}