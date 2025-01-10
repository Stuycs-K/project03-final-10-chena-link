#ifndef FILE_H
#define FILE_H

int set_nonblock(int fd);
int check_writelock(int fd);
void set_writelock(int fd);

#endif