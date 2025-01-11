// A linked list of client information, so that clients know about other clients.

#ifndef CLIENTLIST_H
#define CLIENTLIST_H

typedef struct ClientInfoNode ClientInfoNode;
struct ClientInfoNode {
    int id;
    char name[20];
    ClientInfoNode *next;
};

ClientInfoNode *client_info_new(int id);

ClientInfoNode *get_client_info_for_id(ClientInfoNode *this, int id);
ClientInfoNode *insert_client_list(ClientInfoNode *this, int id);
ClientInfoNode *remove_client_list_by_id(ClientInfoNode *this, int id);
int get_client_list_size(ClientInfoNode *this);
void free_client_list(ClientInfoNode *this);

#endif