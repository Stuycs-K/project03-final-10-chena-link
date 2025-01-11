#include "pipenet.h"

/*
    Declares 2 functions:
    1) A constructor for the struct
    2) The write / read function

    net_args_type_name : the type name of the struct (from typedef)

    internal_name : the suffix for each of the 3 functions
        Convention is to chop off the NetArgs_ prefix of the type name and convert the rest to snake_case
        So the internal name would for NetArgs_DoThis is do_this
*/
#define DECLARE_NET_ARGS(net_args_type_name, internal_name) \
    net_args_type_name *(nargs_##internal_name)();          \
    void *handler_##internal_name(NetBuffer *nb, void *args, int mode);

#ifndef PIPENETEVENTS_H
#define PIPENETEVENTS_H

typedef struct NetArgs_PeriodicHandshake NetArgs_PeriodicHandshake;
struct NetArgs_PeriodicHandshake {
    int id;
};
DECLARE_NET_ARGS(NetArgs_PeriodicHandshake, periodic_handshake)

typedef enum HandshakeErrCode HandshakeErrCode;
enum HandshakeErrCode {
    HEC_SUCCESS = -1,
    HEC_INVALID_ACK,
    HEC_SERVER_IS_FULL,
    HEC_NO_LONGER_ACCEPTING_CONNECTIONS,
};

typedef struct NetArgs_Handshake NetArgs_Handshake;
struct NetArgs_Handshake {
    char *to_client_pipe_name;

    int are_fds_finalized;

    int client_to_server_fd;
    int server_to_client_fd;

    int syn_ack;
    int ack;
    HandshakeErrCode errcode;

    int client_id;
};
DECLARE_NET_ARGS(NetArgs_Handshake, handshake)

typedef struct NetArgs_ClientConnect NetArgs_ClientConnect;
struct NetArgs_ClientConnect {
    char *name;

    int to_client_fd; // The main server will use this to send messages to the clients
};
DECLARE_NET_ARGS(NetArgs_ClientConnect, client_connect)

#endif
