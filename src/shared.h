#ifndef SHARED_H
#define SHARED_H

#define TESTING_CSERVER

#define PIPE_READ 0
#define PIPE_WRITE 1

#define MAX_PLAYER_NAME_CHARACTERS 21
#define MAX_GSERVER_NAME_CHARACTERS 30

#define GSERVER_WKP_NAME_LEN 12

#define DEFAULT_GSERVER_MAX_CLIENTS 2

#define MAX_GSERVER_CLIENTS 4
#define MIN_GSERVER_CLIENTS_FOR_GAME_START 2

#define MAX_CSERVER_GSERVERS 12
#define MAX_CSERVER_CLIENTS 64

// 1/10th second
#define TICK_TIME_MICROSECONDS 100000

#define CSERVER_WKP_NAME "CSERVER"

#define WIDTH 800
#define HEIGHT 800

#define RED 215, 38, 0
#define BLUE 9, 86, 191
#define GREEN 55, 151, 17
#define YELLOW 236, 212, 7
#define WHITE 255, 255, 255

typedef enum GServerStatus GServerStatus;
enum GServerStatus {
    GSS_UNRESERVED = 0,
    GSS_RESERVED,            // Someone reserved the GServer, but they haven't yet joined.
    GSS_WAITING_FOR_PLAYERS, // The person who reserved the server joined, but we haven't reached max_clients
    GSS_SHUTTING_DOWN,       // Everyone left the server.
    GSS_STARTING,            // Host has started countdown
    GSS_GAME_IN_PROGRESS     // We're playing the game
};

#endif
