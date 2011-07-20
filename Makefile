CC=g++
CFLAGS=-I.
LIBS=-lutil

default: daemon.cpp
	$(CC) -o daemon daemon.cpp $(CFLAGS) $(LIBS) -Wno-deprecated -std=gnu++0x 
clean:
	rm -rf daemon 
