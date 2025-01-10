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
