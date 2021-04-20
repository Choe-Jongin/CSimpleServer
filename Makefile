server: server.o
	gcc -g -o myserver server.o

server.o : server.c
	gcc -g -c -o server.o server.c

	
