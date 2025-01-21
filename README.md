[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/Vh67aNdh)
# Uno

## AK-47

Alan Chen, Kevin Lin
       
## Project Description
A pipe-networked multiplayer Uno game. Players connect to a central host server, in which they can create, modify and join game lobbies. When a player creates a lobby, they are granted host permissions and can change the maximum player count, lobby name, or start the game. Players who join during the host's server during this phase can use the downtime to gather their inner strength and mental fortitude to sit through the incoming game of Uno. Or if they realize they are too mentally frail, they can simply disconnect and be brought back to the loving embrace of the central server.

Uno game state is replicated via shared memory, which the server initializes and updates. Players are given copies of the shared memory keys upon joining and use them to locally read and respond to game state updates.
When the game ends, the game server shuts down, and all players are booted back to the central server. Games can end if someone wins, or if everyone realizes their time is better spent elsewhere and willfully disconnects.

## Video Demonstration
[Click me](https://drive.google.com/file/d/1rWW0WJ6KO1SzjpCnKmS39PxEXY3yj6FO/view). It's a Google Drive video. Don't think you can watch it signed into your nycstudents.net account, so watch it signed out. Needed quite a few cuts due to less-than-ideal Uno RNG. Also, Kevin's camera wasn't working.
  
## Instructions

### Dependencies
We render UI with SDL2. To install, run the following commands before compilation: `sudo apt-get install libsdl2-2.0`, `sudo apt-get install libsdl2-dev`, `sudo apt-get install libsdl2-ttf-dev`, and `sudo apt-get install libsdl2-image-dev` (in case it's not already installed).

### Compilation
First, run `make clean` just in case.

Then, run `make` or `make compile` to compile the project. Hopefully, there shouldn't be any problems.

Next, set up a server with `make server`. Wait 3 seconds, just in case.

Finally, to play, open up a new terminal and run `make client`. You should see a "Loading..." message, and a prompt to enter your username should appear after a few seconds.

## Usage
The only terminal-based input required is entering your username when you create a client.

Afterwards, all client inputs are made through the SDL window.

### Creating / Joining Servers
To start playing, you must either create or join a game server. If there's no servers available on the server list (and there won't be at first), click the green "Create Lobby" button on the top right. This should put the client in the waiting menu for the newly created game server.

Now, the client is considered the host of the game server. You should see a list of players and options to disconnect, start the game, or change the maximum players the server will allow. You can't start the game without 2 players, so it's time to get a second client in.

The second client should see the first client's game server on their server list. Click the green "Join" button to join the server. Both clients now should be in the waiting menu and see the updated player list with 2 players. Now, the host can start the game.

If the host decides to not start the game and bails out by disconnecting, the second client becomes the new host.

### Playing the Game
At this point, both players should be in the Uno game. Both players should see their hand and the starting card. The white rectangle to the left of the starting card is the draw button. There are 2 options to disconnect from the game: either close out of the window with the "x" or click the red "Disconnect" button. 

Each turn, players can either play a valid card or draw from the pile. Drawing from the pile is always a valid move, which should hopefully make it easier to make someone win. You can see the other players' number of cards and their names around the board (this only shows up after the first move is played).

When one player has one card remaining, a button to call Uno appears on the bottom right. If another player hits it, the player with one card draws 2 cards. If the player with one card hits it first, they suffer no penalty. 

When a player plays all of their cards, they become the winner (if all players but one disconnect, the remaining player will be deemed the winner), and the server shuts down. All clients should be brought back to the central server and the server list, where they should be able to play another game.

### Interrupts
Interrupting a client will cause it to disconnect from both the game server (if they're in one) and central server. Interrupting the server terminal will shut down the central server and all game servers, causing all clients to exit.

### Other Notes
- You should be able to have multiple 2-player games running at the same time. The server list will also display multiple active game servers.
- 3+ player games were not tested extensively because having 4 terminals and 3 windows open kinda drove us to our breaking points. The only issues we can predict with 3+ player games are players disconnecting mid-game on their turn.
- You will have to double click on windows to click buttons (if you're switching between them to play on different clients). The first click is to put the window into focus. The second click should be fine.
- Not tested with having other computers ssh-ing into one's temp directory and running the binary there, so will it work? That is a great question, and the great answer to the great question is "No."