#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <unistd.h>

#include "../client/baseclient.h"
#include "../shared.h"
#include "gserverwaitui.h"
#include "sdlutil.h"

int titleLabelFontSize = 20;
int normalFontSize = 16;

int isHost = 0;
SDL_Rect disconnectButton = {30, 600, 100, 60};
SDL_Rect startGameButton = {200, 600, 100, 60};
SDL_Rect increaseMaxClientsButton = {400, 600, 60, 60};
SDL_Rect decreaseMaxClientsButton = {500, 600, 60, 60};

void drawServerInfo(SDL_Renderer *renderer, GServerInfo *serverInfo, BaseClient *gclient) {
    SDL_Point buttonPosition = {5, 5};
    renderTextLabel(renderer, serverInfo->name, &buttonPosition, X_LEFT | Y_TOP, NULL, &titleLabelFontSize);

    char playerStatus[32];
    snprintf(playerStatus, sizeof(playerStatus), "%d / %d", serverInfo->current_clients, serverInfo->max_clients);

    SDL_Point playerStatusPosition = {5, 30};
    renderTextLabel(renderer, playerStatus, &playerStatusPosition, X_LEFT | Y_TOP, NULL, &normalFontSize);
}

void drawDisconnectButton(SDL_Renderer *renderer, GServerInfo *serverInfo, BaseClient *gclient) {
    SDL_SetRenderDrawColor(renderer, RED, 255);
    SDL_RenderFillRect(renderer, &disconnectButton);

    SDL_Point textPosition = {disconnectButton.x + disconnectButton.w / 2, disconnectButton.y + disconnectButton.h / 2};
    renderTextLabel(renderer, "Disconnect", &textPosition, X_CENTER | Y_CENTER, NULL, &normalFontSize);
}

void drawHostControls(SDL_Renderer *renderer, GServerInfo *serverInfo, BaseClient *gclient) {
    if (serverInfo->host_id != gclient->client_id) {
        return;
    }

    SDL_SetRenderDrawColor(renderer, GREEN, 255);
    SDL_RenderFillRect(renderer, &startGameButton);

    SDL_Point textPosition = {startGameButton.x + startGameButton.w / 2, startGameButton.y + startGameButton.h / 2};
    renderTextLabel(renderer, "Start Game", &textPosition, X_CENTER | Y_CENTER, NULL, &normalFontSize);

    SDL_SetRenderDrawColor(renderer, BLUE, 255);
    SDL_RenderFillRect(renderer, &increaseMaxClientsButton);

    SDL_Point increaseClientsTextPosition = {increaseMaxClientsButton.x + increaseMaxClientsButton.w / 2, increaseMaxClientsButton.y + increaseMaxClientsButton.h / 2};
    renderTextLabel(renderer, "+1", &increaseClientsTextPosition, X_CENTER | Y_CENTER, NULL, &normalFontSize);

    SDL_SetRenderDrawColor(renderer, BLUE, 255);
    SDL_RenderFillRect(renderer, &decreaseMaxClientsButton);

    SDL_Point decreaseClientsTextPosition = {decreaseMaxClientsButton.x + decreaseMaxClientsButton.w / 2, decreaseMaxClientsButton.y + decreaseMaxClientsButton.h / 2};
    renderTextLabel(renderer, "-1", &decreaseClientsTextPosition, X_CENTER | Y_CENTER, NULL, &normalFontSize);

    SDL_Rect clientCountRect = {450, 550, 60, 60};
    SDL_Point clientCountTextPosition = {clientCountRect.x, clientCountRect.y};

    char textBuffer[64];
    snprintf(textBuffer, sizeof(textBuffer), "Max Clients: %d", serverInfo->max_clients);
    renderTextLabel(renderer, textBuffer, &clientCountTextPosition, X_CENTER | Y_CENTER, NULL, &normalFontSize);
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

        char clientNameBuffer[64];
        strcpy(clientNameBuffer, node->name);
        if (serverInfo->host_id == id) {
            strcat(clientNameBuffer, " (HOST)");
        }

        SDL_Point textPosition = {mainPanel.x, mainPanel.y};
        renderTextLabel(renderer, clientNameBuffer, &textPosition, X_LEFT | Y_TOP, NULL, &normalFontSize);

        node = node->next;
    }
}

void renderGServerWait(SDL_Renderer *renderer, GServerInfo *serverInfo, BaseClient *gclient) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    drawServerInfo(renderer, serverInfo, gclient);
    drawClientList(renderer, serverInfo, gclient);
    drawDisconnectButton(renderer, serverInfo, gclient);
    drawHostControls(renderer, serverInfo, gclient);

    SDL_RenderPresent(renderer);
}

int handleGServerWaitEvent(GServerInfo *serverInfo, BaseClient *gclient) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_MOUSEBUTTONDOWN:
            SDL_Point clickPosition;
            clickPosition.x = event.button.x;
            clickPosition.y = event.button.y;

            if (SDL_PointInRect(&clickPosition, &disconnectButton)) {
                return GSERVER_WAITING_DISCONNECT;
            }

            if (serverInfo->host_id != gclient->client_id) {
                return GSERVER_WAITING_NOTHING;
            }

            if (SDL_PointInRect(&clickPosition, &startGameButton)) {
                return GSERVER_WAITING_START_GAME;
            }

            if (SDL_PointInRect(&clickPosition, &increaseMaxClientsButton)) {
                return serverInfo->max_clients + 1;
            }

            if (SDL_PointInRect(&clickPosition, &decreaseMaxClientsButton)) {
                return serverInfo->max_clients - 1;
            }

            break;
        default:
            break;
        }
    }

    return GSERVER_WAITING_NOTHING;
}