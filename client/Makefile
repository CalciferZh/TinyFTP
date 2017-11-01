CC = gcc
CFLAGS = -Wall
LIB = ../server/src/

all: ./program/librsa.so

./program/librsa.so: rsa.o
	$(CC) $(CFLAGS) -shared -o ./program/librsa.so rsa.o

rsa.o: $(LIB)rsa.h $(LIB)rsa.c
	$(CC) $(CFLAGS) -c -fPIC $(LIB)rsa.c -o rsa.o

clean:
	rm -rf *.o ./program/*.so

