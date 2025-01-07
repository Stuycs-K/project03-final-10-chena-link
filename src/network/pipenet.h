#ifndef PIPENET
#define PIPENET

#define CLIENT_TO_SERVER_PIPE "wkp"

int server_setup();

int server_handshake(int to_client);

int client_handshake(int *to_server);

#endif
