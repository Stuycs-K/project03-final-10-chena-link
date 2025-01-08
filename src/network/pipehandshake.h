#include "pipenet.h"
#include "pipenetevents.h"

#ifndef PIPEHANDSHAKE
#define PIPEHANDSHAKE

#define HANDSHAKE_BUFFER_SIZE 12

int server_setup(char *client_to_server_fifo);

NetEvent *create_handshake_event();
void free_handshake_event(NetEvent *handshake_event);

void server_abort_handshake(NetEvent *handshake_event, HandshakeErrCode errcode);
void server_get_send_fd(NetEvent *handshake_event);
int server_complete_handshake(NetEvent *handshake_event);

void client_setup(char *client_to_server_fifo, NetEvent *handshake_event);
int client_handshake(NetEvent *handshake_event);

#endif
