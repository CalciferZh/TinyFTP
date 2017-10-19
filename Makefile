all: client_app server_app

client_app:
	cd ./client/ && make

server_app:
	cd ./server/ && make

clean:
	cd ./client/ && make clean
	cd ./server/ && make clean

