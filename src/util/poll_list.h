#include <poll.h>

#ifndef POLL_LIST_H
#define POLL_LIST_H

typedef struct PollList PollList;
struct PollList {
    int count;
    int total_requests;
    struct pollfd **requests;
};

PollList *poll_list_new(int initial_size);
struct pollfd *insert_pollfd(PollList *this, int index, int fd);
void remove_pollfd_by_index(PollList *this, int index);
void free_poll_list(PollList *this);

#endif