## Group Members

Kevin Lin, Alan Chen

## Description

A client-server based multiplayer Uno. Players can join or create games with a lobby system.

## Usage

First, a machine must run a centralized server, which will set up game servers locally on that machine. When a client connects, they may either join an existing game server (from a shared memory server list) or create a new one. If they wish to create one, the client can send server configurations (i.e. password, maximum players) to the central server, which will then create a game server. Other clients can then view details of and join the created game server in the server list. The host is responsible for starting the game.

Clients will render the game using SDL2, and process and send keyboard and mouse inputs to the server. The game server will be terminal-based and modify shared memory, with potentially the option to input commands to modify game rules (or cheat). Clients will read from shared memory to update and display local game state.

## Concepts Used

1. Memory allocation

2. Shared memory to record game state and game server state

3. Pipes to perform handshakes and then handle client to server communication

4. Processes, specifically forking, to handle multiple game servers

5. Signals to handle disconnects

## Responsibilities

The project can be broken down into general client-server networking (Alan) and gameplay features (Kevin).

## Algorithms and Data Structures

* A queue to order turns. The player at the front of the queue is next to go, and when it’s their turn, they get moved to the back of the queue.

## Timeline

### Thursday, January 9

* Pipe-based networking with a non-forking game server

* Basic, working Uno game logic

### Monday, January 13

* Rendering game state with SDL2

* Handling client input

* Networking game state

* Centralized forking server that creates game servers

* Joining game servers

### Thursday, January 16

* Server list and related UI

* Game server modifications (e.g. player count)

### Monday, January 20

* UI and graphical polish

* Miscellaneous game features (e.g. more game rules such as 7-0)
