all: server

server:
	cd ./server/ && make

test:
	cd ./server/ && make test

clean:
	cd ./server/ && make clean

