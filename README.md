[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/Vh67aNdh)
# Uno

### AK-47

Alan Chen, Kevin Lin
       
### Project Description
A pipe-networked multiplayer Uno game. Players connect to a central host server, in which they can create, modify and join game lobbies. When a player creates a lobby, they are granted host permissions and can change the maximum player count, lobby name, or start the game. Players who join during the host's server during this phase can use the downtime to gather their inner strength and mental fortitude to sit through the incoming game of Uno. Or if they realize they are too mentally frail, they can simply disconnect and be brought back to the loving embrace of the central server.

Uno game state is replicated via shared memory, which the server initializes and updates. Players are given copies of the shared memory keys upon joining and use them to locally read and respond to game state updates.
When the game ends, the game server shuts down, and all players are booted back to the central server. Games can end if someone wins, or if everyone realizes their time is better spent elsewhere and willfully disconnects.

### Video Demonstration
  
### Instructions

#### Dependencies
We render UI with SDL2. To install, run the following commands before compilation: `sudo apt-get install libsdl2-2.0`, `sudo apt-get install libsdl2-dev`, and `sudo apt-get install libsdl2-ttf-dev`.

#### Compilation
First, run `make clean` just in case.
Then, run `make` or `make compile` to compile the project. Hopefully, there shouldn't be any problems.
Next, set up a server with `make server`.
Finally, to play, open up a new terminal and run `make client`.

#### Usage