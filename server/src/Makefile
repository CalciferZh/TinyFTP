CC = gcc
CFLAGS = -Wall

all: server

server: server.o utils.o rsa.o commands.o
	$(CC) $(CFLAGS) server.o utils.o rsa.o commands.o -o ../server -lpthread

server.o: server.c
	$(CC) $(CFLAGS) -c server.c -o server.o

commands.o: commands.h commands.c
	$(CC) $(CFLAGS) -c commands.c -o commands.o

utils.o: utils.h utils.c
	$(CC) $(CFLAGS) -c utils.c -o utils.o

rsa.o: rsa.h rsa.c
	$(CC) $(CFLAGS) -c rsa.c -o rsa.o

test: utils.o test.c rsa.o
	clear
	$(CC) $(CFLAGS) -c test.c -o test.o
	$(CC) $(CFLAGS) test.o utils.o rsa.o -o utest -lpthread
	./utest
	rm -rf test.o utest

clean:
	rm -rf *.o ../server
