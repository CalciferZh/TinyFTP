CC = gcc
CFLAGS = -Wall

all: server_app

server_app: server.o utils.o
	$(CC) server.o utils.o -o ../server_app

server.o:
	$(CC) $(CFLAGS) -c server.c -o server.o

utils.o:
	$(CC) $(CFLAGS) -c utils.c -o utils.o

clean:
	rm -rf *.o ../server_app
