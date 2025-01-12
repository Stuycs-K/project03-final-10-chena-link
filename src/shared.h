#ifndef SHARED_H
#define SHARED_H

#define TESTING_CSERVER

#define PIPE_READ 0
#define PIPE_WRITE 1

#define MAX_PLAYER_NAME_CHARACTERS 21
#define MAX_GSERVER_NAME_CHARACTERS 30

#define GSERVER_WKP_NAME_LEN 12

#define DEFAULT_GSERVER_MAX_CLIENTS 2
#define MAX_GSERVER_CLIENTS 8
#define MAX_CSERVER_GSERVERS 12
#define MAX_CSERVER_CLIENTS 64

#define CONNECTION_IS_FREE 1
#define CONNECTION_IS_USED 0

// 1/10th second
#define TICK_TIME_MICROSECONDS 100000

#define CSERVER_WKP_NAME "CSERVER"

typedef enum GServerStatus GServerStatus;
enum GServerStatus {
    GSS_UNRESERVED,
    GSS_WAITING_FOR_PLAYERS, // Not reached max_clients
    GSS_FULL,                // Server has reached max_clients
    GSS_STARTING,            // Host has started countdown
    GSS_GAME_IN_PROGRESS     // We're playing the game
};

#endif