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

## Kevin Lin

### 2024-01-06 - Added new file and functions
Added game.c which will contain game rules like drawing and playing cards. Spent 40 minutes.

### 2024-01-07 - Brief description