server: server.o
	gcc -o server server.o

server.o : server.c
	gcc -c -o server.o server.c

	
