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