#ifndef PIPEHANDSHAKE
#define PIPEHANDSHAKE

#define HANDSHAKE_BUFFER_SIZE 12

int server_setup(char *client_to_server_fifo);

int server_handshake(int to_client);

void client_handshake(char *client_to_server_fifo, int *fd_pair)

#endif
