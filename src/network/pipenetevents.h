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
        Convention is to convert the type name (in PascalCase) to snake_case
        So the internal name would for DoThis is do_this
*/
#define DECLARE_NET_ARGS(net_args_type_name, internal_name)             \
    net_args_type_name *(nargs_##internal_name)();                      \
    void *handler_##internal_name(NetBuffer *nb, void *args, int mode); \
    void destroy_##internal_name(void *args);

#ifndef PIPENETEVENTS_H
#define PIPENETEVENTS_H

typedef enum HandshakeErrCode HandshakeErrCode;
enum HandshakeErrCode {
    HEC_SUCCESS = -1,
    HEC_INVALID_ACK,
    HEC_SERVER_IS_FULL,
    HEC_NO_LONGER_ACCEPTING_CONNECTIONS,
};

/*
    Represents the client-server handshake.
*/
typedef struct Handshake Handshake;
struct Handshake {
    char *client_name;         // The name the client
    char *to_client_pipe_name; // The name of the client's PP (stringified client PID)

    int are_fds_finalized; // Used by the connection handler to determine whether to read / write the FDs.
                           // Default: 0 because the client and server overwriting each other's FDs is a pretty big no-no.

    int client_to_server_fd; // Self-explanatory
    int server_to_client_fd; // Self-explanatory

    int syn_ack;              // The SYN-ACK the server sent the client
    int ack;                  // The ACK the client sent the server
    HandshakeErrCode errcode; // If the handshake failed, the reason (see HandshakeErrCode)

    int client_id;
};
DECLARE_NET_ARGS(Handshake, handshake)

/*
    The list of clients connected to the server. The server sends this whenever a client leaves / joins.
*/
typedef struct ClientList ClientList;
struct ClientList {
    int local_client_id;       // Which client
    ClientInfoNode *info_list; // Linked list of connected clients and their names
};
DECLARE_NET_ARGS(ClientList, client_list)

/*
    First sent by a client to ask for a GServer.
    The CServer responds with the ID of the GServer.
*/
typedef struct ReserveGServer ReserveGServer;
struct ReserveGServer {
    int gserver_id; // The ID of the GServer the client reserved. -1 if no valid GServer could be reserved
};
DECLARE_NET_ARGS(ReserveGServer, reserve_gserver)

// Used for client modifying GServer properties
typedef struct GServerConfig GServerConfig;
struct GServerConfig {
    int gserver_id;
    char name[MAX_GSERVER_NAME_CHARACTERS];
    int max_clients;
};
DECLARE_NET_ARGS(GServerConfig, gserver_config)

/*
    Information about a GServer. Sent to CServers from GServers when clients join / disconnect.
*/
typedef struct GServerInfo GServerInfo;
struct GServerInfo {
    int id;               // The GServer ID
    GServerStatus status; // See GServerStatus in shared.h
    int current_clients;  // How many clients are currently connected
    int max_clients;      // Maximum number of clients

    char name[MAX_GSERVER_NAME_CHARACTERS]; // The visible name of the server
    char wkp_name[GSERVER_WKP_NAME_LEN];    // The WKP name (what clients use to join)
};
DECLARE_NET_ARGS(GServerInfo, gserver_info)

/*
    List of GServerInfo. Sent to clients from CServers when the CServer receives an update
    to a GServerInfo struct from a GServer.
*/
typedef GServerInfo *GServerInfoList;

DECLARE_NET_ARGS(GServerInfoList, gserver_info_list)

#endif
