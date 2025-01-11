#include <stdlib.h>

#include "poll_list.h"

PollList *poll_list_new(int initial_size) {
    PollList *this = malloc(sizeof(PollList));

    this->count = 0;
    this->total_requests = initial_size;
    this->requests = malloc(sizeof(struct pollfd *) * initial_size);

    return this;
}

struct pollfd *insert_pollfd(PollList *this, int index, int fd) {
    struct pollfd *request = malloc(sizeof(struct pollfd));
    request->fd = fd;

    if (this->requests[index] != NULL) {
        free(this->requests[index]);
        this->requests[index] = NULL;
    }

    this->requests[index] = request;
    this->count++;

    return request;
}

void remove_pollfd_by_index(PollList *this, int index) {
    if (this->requests[index] == NULL) {
        return;
    }

    free(this->requests[index]);
    this->requests[index] = NULL;
}

void free_poll_list(PollList *this) {
    for (int i = 0; i < this->total_requests; ++i) {
        remove_pollfd_by_index(this, i);
    }

    free(this->requests);
    free(this);
}