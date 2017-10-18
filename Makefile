all: client_app server_app

client_app: ./client/client.c
	gcc ./client/client.c -o client_app

server_app: ./server/server.c
	gcc ./server/server.c -o server_app

clean:
	rm -rf client_app server_app

