CC=gcc
CFLAGS=-I.
LIBS=-lutil

default: daemon.c
	$(CC) -o test_hashmap test_hashmap.cpp $(CFLAGS) $(LIBS) -Wno-deprecated -std=gnu++0x
	#$(CC) -o testinput testinput.c $(CFLAGS) $(LIBS)

clean:
	rm -rf daemon testinput
