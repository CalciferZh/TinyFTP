all: ./server/server ./client/client

./server/server:
	cd ./server/src/ && make all

./client/client:
	cd ./client/ && make all

clean:
	cd ./server/src && make clean
	cd ./client/ && make clean
