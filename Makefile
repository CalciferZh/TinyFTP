all: ./server/server ./client/client

./server/server:
	cd ./server/ && make all

./client/client:
	cd ./client/ && make all

clean:
	cd ./server/ && make clean
	cd ./client/ && make clean

