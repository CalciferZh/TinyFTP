all: server_app

server_app:
	cd ./server/ && make

test:
	cd ./server/ && make test

clean:
	cd ./server/ && make clean

