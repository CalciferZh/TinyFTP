CC = gcc
CFLAGS = -Wall

all: server_app

server_app: server.o utils.o
	$(CC) $(CFLAGS) server.o utils.o -o ../server_app

server.o: server.c
	$(CC) $(CFLAGS) -c server.c -o server.o

utils.o: utils.h utils.c
	$(CC) $(CFLAGS) -c utils.c -o utils.o

test: utils.o test.c
	clear
	$(CC) $(CFLAGS) -c test.c -o test.o
	$(CC) $(CFLAGS) test.o utils.o -o utest
	./utest
	rm -rf test.o utest

clean:
	rm -rf *.o ../server_app
