#include "pipenet.h"

#include "../shared.h"
#include "clientlist.h"

/*
    Use this macro after every NetArgs typedef.

    Declares 2 functions:
    1) A constructor for the struct
    2) The write / read function
    3) A destructor for the struct

    Example: DECLARE_NET_ARGS(Handshake, handshake) generates:

        Handshake *nargs_handshake();
        void *handler_handshake(NetBuffer *nb, void *args, int mode);
        void free_handshake(Handshake *nargs);

    net_args_type_name : the type name of the struct (from typedef)

    internal_name : the suffix for each of the 3 functions
        Convention is to chop off the NetArgs_ prefix of the type name and convert the rest to snake_case
        So the internal name would for NetArgs_DoThis is do_this
*/
#define DECLARE_NET_ARGS(net_args_type_name, internal_name)             \
    net_args_type_name *(nargs_##internal_name)();                      \
    void *handler_##internal_name(NetBuffer *nb, void *args, int mode); \
    void destroy_##internal_name(void *args);

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
typedef struct Handshake Handshake;
struct Handshake {
    char *client_name;
    char *to_client_pipe_name;

    int are_fds_finalized;

    int client_to_server_fd;
    int server_to_client_fd;

    int syn_ack;
    int ack;
    HandshakeErrCode errcode;

    int client_id;
};
DECLARE_NET_ARGS(Handshake, handshake)

// The list of clients connected to the server. The server sends this whenever a client leaves / joins.
typedef struct ClientList ClientList;
struct ClientList {
    int local_client_id;
    ClientInfoNode *info_list; // Linked list of connected clients and their names
};
DECLARE_NET_ARGS(ClientList, client_list)

/*
    First sent by a client to ask for a GServer.
    The CServer responds with the ID of the GServer.
*/
typedef struct ReserveGServer ReserveGServer;
struct ReserveGServer {
    int gserver_id;
};

// Used for client modifying GServer properties
typedef struct GServerConfig GServerConfig;
struct GServerConfig {
    int gserver_id;
    char name[MAX_GSERVER_NAME_CHARACTERS];
    int max_clients;
};
DECLARE_NET_ARGS(GServerConfig, gserver_config)

typedef struct GServerInfo GServerInfo;
struct GServerInfo {
    int id;
    int status; // The GServerStatus enum
    int current_clients;
    int max_clients;

    char name[MAX_GSERVER_NAME_CHARACTERS];
    char wkp_name[GSERVER_WKP_NAME_LEN];
};
DECLARE_NET_ARGS(GServerInfo, gserver_info)

typedef struct GServerInfoList GServerInfoList;
struct GServerInfoList {
    GServerInfo **gserver_list;
};
DECLARE_NET_ARGS(GServerInfoList, gserver_info_list)

#endif
