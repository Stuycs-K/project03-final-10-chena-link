#include "pipenet.h"

#ifndef PIPENETEVENTS_H
#define PIPENETEVENTS_H

typedef struct NetArgs_PeriodicHandshake NetArgs_PeriodicHandshake;
struct NetArgs_PeriodicHandshake {
    int id;
};

void send_periodic_handshake(NetBuffer *nb, void *args);
void *recv_periodic_handshake(NetBuffer *nb, void *args);

typedef enum HandshakeErrCode HandshakeErrCode;
enum HandshakeErrCode {
    HEC_SUCCESS = -1,
    HEC_INVALID_ACK,
    HEC_SERVER_IS_FULL,
    HEC_NO_LONGER_ACCEPTING_CONNECTIONS,
};

typedef struct NetArgs_InitialHandshake NetArgs_InitialHandshake;
struct NetArgs_InitialHandshake {
    char *to_client_pipe_name;

    int client_to_server_fd;
    int server_to_client_fd;

    int syn_ack;
    int ack;
    HandshakeErrCode errcode;

    int client_id;
};

void send_initial_handshake(NetBuffer *nb, void *args);
void *recv_initial_handshake(NetBuffer *nb, void *args);

typedef struct NetArgs_ClientConnect NetArgs_ClientConnect;
struct NetArgs_ClientConnect {
    char *name;
};

void send_client_connect(NetBuffer *nb, void *args);
void *recv_client_connect(NetBuffer *nb, void *args);

typedef struct NetArgs_CardCount NetArgs_CardCount;
struct NetArgs_CardCount {
    int card_count;
};

#endif
