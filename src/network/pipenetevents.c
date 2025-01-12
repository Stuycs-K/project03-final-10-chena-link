/*
    For each protocol:

    Build a constructor function with DECLARE_CONSTRUCTOR. Make sure to close it with END_CONSTRUCTOR
    Build a handler function with DECLARE_HANDLER. Make sure to close it with END_HANDLER
*/

#include <stdio.h>

#include "../shared.h"
#include "pipenetevents.h"

// The variable "nargs" is available and has the correct type
#define DECLARE_CONSTRUCTOR(net_args_type_name, internal_name) \
    net_args_type_name *(nargs_##internal_name)() {            \
        net_args_type_name *nargs = malloc(sizeof(net_args_type_name));

#define END_CONSTRUCTOR() \
    return nargs;         \
    }

// The variable "nargs" is available and has the correct type
#define DECLARE_HANDLER(net_args_type_name, internal_name)               \
    void *handler_##internal_name(NetBuffer *nb, void *args, int mode) { \
        if (args == NULL) {                                              \
            args = nargs_##internal_name();                              \
        }                                                                \
        net_args_type_name *nargs = args;

// Read / write a non-pointer field
#define VALUE(field)                      \
    if (mode == 0) {                      \
        NET_BUFFER_WRITE_VALUE(nb, field) \
    } else {                              \
        NET_BUFFER_READ_VALUE(nb, field)  \
    }

// Read / write a pointer field (needs size)
#define PTR(field, size)                  \
    if (mode == 0) {                      \
        NET_BUFFER_WRITE(nb, field, size) \
    } else {                              \
        NET_BUFFER_READ(nb, field, size)  \
    }

// Read / write a string (char *) field
#define STRING(field)                      \
    if (mode == 0) {                       \
        NET_BUFFER_WRITE_STRING(nb, field) \
    } else {                               \
        NET_BUFFER_READ_STRING(nb, field)  \
    }

#define END_HANDLER() \
    return nargs;     \
    }

//============================================================

DECLARE_CONSTRUCTOR(NetArgs_PeriodicHandshake, periodic_handshake) {
    nargs->id = 69420;
}
END_CONSTRUCTOR()

DECLARE_HANDLER(NetArgs_PeriodicHandshake, periodic_handshake) {
    VALUE(nargs->id);
}
END_HANDLER()

//============================================================

DECLARE_CONSTRUCTOR(NetArgs_Handshake, handshake) {
    nargs->ack = -1;
    nargs->errcode = -1;
    nargs->syn_ack = -1;
    nargs->are_fds_finalized = 0;
    nargs->client_to_server_fd = -1;
    nargs->server_to_client_fd = -1;
    nargs->client_id = -1;
    nargs->to_client_pipe_name = calloc(sizeof(char), 13);
}
END_CONSTRUCTOR()

DECLARE_HANDLER(NetArgs_Handshake, handshake) {
    VALUE(nargs->syn_ack);
    VALUE(nargs->ack);
    VALUE(nargs->errcode);
    VALUE(nargs->are_fds_finalized);

    if (nargs->are_fds_finalized) {
        VALUE(nargs->client_to_server_fd);
        VALUE(nargs->server_to_client_fd);
    }

    STRING(nargs->to_client_pipe_name);
    VALUE(nargs->client_id);
}
END_HANDLER()

//============================================================

DECLARE_CONSTRUCTOR(ClientList, client_list) {
    nargs->local_client_id = -1;
    nargs->info_list = NULL;
}
END_CONSTRUCTOR()

DECLARE_HANDLER(ClientList, client_list) {
    VALUE(nargs->local_client_id);

    if (mode == 0) {
        // Writing the linked list: First, write the size and then write the properties of each node
        ClientInfoNode *node = nargs->info_list;

        int total_clients = get_client_list_size(node);
        VALUE(total_clients);

        while (node != NULL) {
            VALUE(node->id);
            STRING(node->name);
            node = node->next;
        }
    } else {
        // First, free the existiing linked list.
        if (nargs->info_list != NULL) {
            free_client_list(nargs->info_list);
        }

        int total_clients;
        VALUE(total_clients);

        for (int i = 0; i < total_clients; ++i) {
            int client_id;
            VALUE(client_id);

            nargs->info_list = insert_client_list(nargs->info_list, client_id);
            STRING(nargs->info_list->name)
        }
    }
}
END_HANDLER()

DECLARE_CONSTRUCTOR(GServerInfo, gserver_info) {
    nargs->id = -1;
    nargs->status = 0;
    nargs->current_clients = 0;
    nargs->max_clients = DEFAULT_GSERVER_MAX_CLIENTS;

    // Null out the strings
    nargs->name[0] = 0;
    nargs->wkp_name[0] = 0;
}
END_CONSTRUCTOR()

DECLARE_HANDLER(GServerInfo, gserver_info) {
    VALUE(nargs->id);
    VALUE(nargs->status);
    VALUE(nargs->current_clients);
    VALUE(nargs->max_clients);
    STRING(nargs->name);
    STRING(nargs->wkp_name);
}
END_CONSTRUCTOR()

DECLARE_CONSTRUCTOR(GServerInfoList, gserver_info_list) {
    nargs->gserver_list = malloc(sizeof(GServerInfo *) * MAX_CSERVER_GSERVERS);
}
END_CONSTRUCTOR()

DECLARE_HANDLER(GServerInfoList, gserver_info_list) {
    for (int i = 0; i < MAX_CSERVER_GSERVERS; ++i) {
        GServerInfo *server_info = nargs_gserver_info();
        handler_gserver_info(nb, server_info, mode);
        nargs->gserver_list[i] = server_info;
    }
}
END_CONSTRUCTOR()