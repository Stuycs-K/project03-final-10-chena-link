#include "../network/pipenetevents.h"

#ifndef GSERVERLIST_H
#define GSERVERLIST_H

typedef struct GServerList GServerList;
struct GServerList {
    int visible_server_count;
    int total_servers;
    GServerInfo **gserver_info_list;
};

GServerList *gserver_list_new();

int update_gserver_list(GServerList *this, GServerInfo **server_info_list);

void print_gserver_list(GServerList *this);

#endif