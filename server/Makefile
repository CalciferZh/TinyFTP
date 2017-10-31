CC = gcc
CFLAGS = -Wall

all: server 

server: server.o utils.o rsa.o
	$(CC) $(CFLAGS) server.o utils.o rsa.o -o ./server -lpthread

server.o: server.c
	$(CC) $(CFLAGS) -c server.c -o server.o

utils.o: utils.h utils.c
	$(CC) $(CFLAGS) -c utils.c -o utils.o -lpthread

rsa.o: rsa.h rsa.c
	$(CC) $(CFLAGS) -c rsa.c -o rsa.o

test: utils.o test.c rsa.o
	clear
	$(CC) $(CFLAGS) -c test.c -o test.o
	$(CC) $(CFLAGS) test.o utils.o rsa.o -o utest -lpthread
	./utest
	rm -rf test.o utest

clean:
	rm -rf *.o ../server_app ./server
