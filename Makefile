CC = gcc
CFLAGS = -g -Wall -pedantic

all: client server list


client: client.o
	$(CC) $(CFLAGS) client.o -L/usr/local/ssl/lib -lcrypto -o client

client2.o: client.c
	$(CC) $(CFLAGS) -I/usr/local/ssl/include/ -c client.c

server: server.o list
	$(CC) $(CFLAGS) server.o -L/usr/local/ssl/lib -lcrypto -L./list -llist -o server

server.o: server.c
	$(CC) $(CFLAGS) -I/usr/local/ssl/include/ -I./list -c server.c

list:
	cd list && make


clean:
	rm -f *.o *.a client server
	cd list && make clean

# not actually filenames
.PHONY: all clean list
