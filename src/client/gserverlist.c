#include <stdio.h>

#include "../shared.h"
#include "gserverlist.h"

GServerList *gserver_list_new() {
    GServerList *this = malloc(sizeof(GServerList));

    this->visible_server_count = 0;
    this->total_servers = MAX_CSERVER_GSERVERS;
    this->gserver_info_list = malloc(sizeof(GServerInfo *) * this->total_servers);

    return this;
}

int update_gserver_list(GServerList *this, GServerInfo **server_info_list) {
    int did_change = 0;

    for (int i = 0; i < this->total_servers; ++i) {
        GServerInfo *recv_info = server_info_list[i];
        GServerInfo *stored_info = this->gserver_info_list[i];

        // Only fields that change
        printf("yueah\n");
        stored_info->current_clients = recv_info->current_clients;

        stored_info->max_clients = recv_info->max_clients;

        stored_info->status = recv_info->status;

        strcpy(stored_info->name, recv_info->name);
    }

    return did_change;
}

void print_gserver_list(GServerList *this) {
    printf("======= Game Server List\n");
    printf("======= Servers: %d\n", this->visible_server_count);

    for (int i = 0; i < this->total_servers; ++i) {
        GServerInfo *info = this->gserver_info_list[i];
        if (info->status == 0) {
            continue;
        }

        printf("[%d] %s: %d / %d\n", info->id, info->name, info->current_clients, info->max_clients);
    }
}