#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "clientlist.h"

/*
    Create a new ClientListNode object

    PARAMS:
        int id : which client

    RETURNS: the new ClientListNode
*/
ClientInfoNode *client_info_new(int id) {
    ClientInfoNode *this = malloc(sizeof(ClientInfoNode));
    this->id = id;
    this->name[0] = 0;
    this->next = NULL;
    return this;
}

/*
    Gets the node with the given client ID

    PARAMS:
        ClientInfoNode *this : the linked list
        int id : which client

    RETURNS: the ClientListNode, if found, otherwise NULL
*/
ClientInfoNode *get_client_info_for_id(ClientInfoNode *this, int id) {
    while (this != NULL) {
        if (this->id == id) {
            return this;
        }
        this = this->next;
    }
    return NULL;
}

/*
    Inserts a ClientListNode to the front of the list

    PARAMS:
        ClientInfoNode *this : the linked list
        int id : the new ClientListNode's ID

    RETURNS: the head of the new linked list
*/
ClientInfoNode *insert_client_list(ClientInfoNode *this, int id) {
    ClientInfoNode *new_node = client_info_new(id);
    new_node->next = this;
    return new_node;
}

/*
    Removes a ClientListNode from the list given its ID

    PARAMS:
        ClientInfoNode *this : the linked list
        int id : the client ID to remove

    RETURNS: the head of the new linked list
*/
ClientInfoNode *remove_client_list_by_id(ClientInfoNode *this, int id) {
    ClientInfoNode *current = this;
    ClientInfoNode *previous = NULL;

    if (this == NULL) {
        return NULL;
    }

    // Remove head
    if (current->id == id) {
        this = current->next;
        free(current);
        return this;
    }

    while (current != NULL) {
        if (current->id == id) {
            if (previous != NULL) {
                previous->next = current->next;
            }

            free(current);

            return this;
        }

        previous = current;
        current = current->next;
    }

    return this;
}

/*
    Returns a copy of the client linked list

    PARAMS:
        ClientInfoNode *this : the linked list to copy

    RETURNS: the head of the copied linked list
*/
ClientInfoNode *copy_client_list(ClientInfoNode *this) {
    if (this == NULL) {
        return NULL;
    }

    // Copy head
    ClientInfoNode *new_head = malloc(sizeof(ClientInfoNode));
    new_head->id = this->id;
    strcpy(new_head->name, this->name);

    // And the rest of the list
    ClientInfoNode *current = new_head;
    this = this->next;
    while (this != NULL) {
        current->next = malloc(sizeof(ClientInfoNode));
        current = current->next;

        current->id = this->id;
        strcpy(current->name, this->name);

        this = this->next;
    }
    current->next = NULL; // Set last element as tail

    return new_head;
}

/*
    Returns the number of nodes in the linked list

    PARAMS:
        ClientInfoNode *this : the linked list

    RETURNS: the linked list size
*/
int get_client_list_size(ClientInfoNode *this) {
    int size = 0;
    while (this != NULL) {
        size++;
        this = this->next;
    }
    return size;
}

/*
    Prints the linked list to the terminal. Will be unused soon.

    PARAMS:
        ClientInfoNode *this : the linked list

    RETURNS: none
*/
void print_client_list(ClientInfoNode *this) {
    printf("|| CLIENT LIST ||\n");
    printf("Size: %d\n", get_client_list_size(this));

    while (this != NULL) {
        printf("[%d]: %s\n", this->id, this->name);
        this = this->next;
    }
}

void free_client_list(ClientInfoNode *this) {
    while (this != NULL) {
        ClientInfoNode *temp = this;
        this = temp->next;
        free(temp);
    }
}