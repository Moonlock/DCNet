# Angela Stankowski
# ans723
# 11119985
#

CC = gcc
CFLAGS = -g -Wall -pedantic

all: client server


client: client.o
	$(CC) $(CFLAGS) client.o -o client

client.o: client.c
	$(CC) $(CFLAGS) -c client.c

server: server.o
	$(CC) $(CFLAGS) server.o -L./list -llist -o server

server.o: server.c
	cd list && make
	$(CC) $(CFLAGS) -I./list -c server.c


clean:
	rm -f *.o *.a client server
	cd list && make clean

# not actually filenames
.PHONY: all clean
