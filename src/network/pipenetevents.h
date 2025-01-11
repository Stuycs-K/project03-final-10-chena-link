#include "pipenet.h"

#include "clientlist.h"

/*
    Use this macro after every NetArgs typedef.

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

// Client-server handshake for GServers and CServers.
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

// The list of clients connected to the server. The server sends this whenever a client leaves / joins.
typedef struct ClientList ClientList;
struct ClientList {
    int local_client_id;
    ClientInfoNode *info_list; // Linked list of connected clients and their names
};
DECLARE_NET_ARGS(ClientList, client_list)

typedef struct GServerState GServerState;
struct GServerState {
};

// A client wants to reserve
typedef struct ReserveGServer ReserveGServer;
struct ReserveGServer {
    int gserver_id;
};

typedef struct GServerInfo GServerInfo;
struct GServerInfo {
    int id;
    int visible;
    int current_clients;
    int max_clients;

    char name[20];
    char wkp_name[10];
};
DECLARE_NET_ARGS(GServerInfo, gserver_info)

#endif
