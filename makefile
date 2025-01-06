default: compile

compile: client server

client: client.o
	@gcc -o client client.o

server: server.o
	@gcc -o server server.o

client.o: src/client.c
	@gcc -c src/client.c

server.o: src/server.c
	@gcc -c src/server.c
