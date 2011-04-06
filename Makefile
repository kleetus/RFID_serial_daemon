CC=gcc
CFLAGS=-I.
LIBS=-lutil

default: daemon.c
	$(CC) -o daemon daemon.c $(CFLAGS) $(LIBS)
	$(CC) -o testinput testinput.c $(CFLAGS) $(LIBS)

clean:
	rm -rf daemon testinput
