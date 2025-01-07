/*
    GServer, or game server handles players for a single game instance.
*/

#ifndef GSERVER
#define GSERVER

struct GServer {
};

char *get_client_to_server_fifo_name();

void gserver_init();

#endif