CC=gcc
CFLAGS=-I.
LIBS=-lutil

daemon: daemon.c
	$(CC) -o daemon daemon.c $(CFLAGS) $(LIBS)

clean:
	rm -rf daemon
