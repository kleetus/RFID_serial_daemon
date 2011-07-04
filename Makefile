CC=g++
CFLAGS=-I.
LIBS=-lutil -lre2

default: daemon.c
	$(CC) -o daemon daemon.cpp $(CFLAGS) $(LIBS) -Wno-deprecated -std=gnu++0x 
clean:
	rm -rf daemon testinput test_hashmap
