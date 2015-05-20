# Angela Stankowski
# ans723
# 11119985
#

CC = gcc
CFLAGS = -g -Wall -pedantic

LISTOBJS=list_adders.o list_movers.o list_removers.o

all: liblist.a

liblist.a: $(LISTOBJS)
	ar rvs liblist.a $(LISTOBJS)

list_adders.o: list_adders.c
	$(CC) $(CFLAGS) -c list_adders.c

list_movers.o: list_movers.c
	$(CC) $(CFLAGS) -c list_movers.c

list_removers.o: list_removers.c
	$(CC) $(CFLAGS) -c list_removers.c


clean:
	rm -f *.o *.a

# not actually filenames
.PHONY: all clean
