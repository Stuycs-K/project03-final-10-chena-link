# Dev Log:

This document must be updated daily by EACH group member.

## Alan Chen

### 2024-01-06 - Project setup
* Write makefile, create skeleton network files and copy over pipe networking lab, main.c. Done in class.
* Finish the makefile. ~2 hours because makefiles are really dumb.
* Client-server handshake works. 20 minutes.
* GServer and GSubserver basics. 1 hour.

### 2024-01-07 - Pipe network messaging
* Improve pipe network macros. Done in class.
* Implement basic messaging by queueing NetEvents of different protocols into a NetEventQueue. After a fixed timestep, they're written to a NetBuffer, which is exported with write. 1.5 hours.
* Rewrite handshake to allow full servers to boot clients during the handshake process. 1 hour.

### 2024-01-08 - Better servers
* Setup CServer (far from finished), improve handshaking. Done in class.
* Reform packet structure to include a VLQ header. 30 minutes.
* Get GServers to run a game process that reads network messages from all GSubservers. 3 hours due to the most terrible segfault I've ever had to deal with.

### 2024-01-09 - I wish I knew about poll
* Set server reads to nonblocking so they can be put in the same loop as everything else. All of this will be rewritten in favor of using poll() tomorrow. 1 hour.
* Begin refactoring components of GServer into base server modules. 1 hour.

### 2024-01-10 - Overhaul
* Servers no longer use subservers. Instead, they use poll. Much cleaner. 2 hours.
* Finish refactoring GServer, started yesterday. Also refactor the pipenet library so that there's less manual labor involved. 2 hours.
* Begin writing CServer. 20 minutes.

### 2024-01-11 - CServer
* Implement a networked client list so that clients can know which other clients are connected. 1 hour.
* Cleanup client.c by offloading network functions into the baseclient module. 2 hours.
* Implement server list display for the client. 1 hour.
* Clients can reserve and join servers on the server list. 2 hours.

### 2024-01-12 - Long overdue documentation
* Document mainserver, cserver, gserver, baseclient, pipenet, and pipenetevents. 1.5 hours.

### 2024-01-13 - Disconnects and documentation
* Document CServer. Done in class.
* More gracefully handle client disconnections. GServers are shut down properly when there's no players in them. 1 hour.
* SDL2 installed and should be available. 15 minutes.

### 2024-01-14 - Configuration and bug fixing
* Begin implementing clients configuring GServers (max players, server name). Done in class.
* Fix a nasty bug with server_send_to_all. 30 minutes.

### 2025-01-15 - Configuration completed
* General bug fixes with server sending duplicate events. Done in class.
* Finish implementing GServerConfig. A host client can now edit GServer properties and start the game. Disconnecting transfers host permissions to the next client. 1 hour.

### 2025-01-16 - Even more bug fixes
* Try to track down a mysterious joining bug. Done in class.
* Document Client in clientconnection.c. 5 minutes.
* Fix a double free bug with the client. 10 minutes.

## Kevin Lin

### 2024-01-06 - Added new file and functions
Added game.c which will contain game rules like drawing and playing cards. Spent 40 minutes.

### 2024-01-07 - Game functions
Added uno game functions to game server, added card array to server struct. Done in class

### 2024-01-08 - Client using game functions
Clients can use functions and see cards they have. 1.5 hour
Added shared memory to see last card placed. 20 minutes
Added dynamic deck size.10 minutes

### 2024-01-09 - Client functions to send card count to server
Added functions to send card count to server. Done in class

### 2024-01-10 - Client server interaction functions
Added functions for client to send card count to server.

### 2024-01-13 - Setting up shmid and gamedata send
Added shmid to server(sending it causes client to segfault)
Added client sending cards
Added server sending cards. 2 hours

### 2024-01-14 - Changing shared memory
Changed shared memory to have id and card
SHMId gets sent at the start of connection
Tried to fix bug for client turns. 3 hours
