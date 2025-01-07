#ifndef PIPEHANDSHAKE
#define PIPEHANDSHAKE

#define HANDSHAKE_BUFFER_SIZE 12

int server_setup();

int server_handshake(int to_client);

int client_handshake(int *to_server);

#endif
