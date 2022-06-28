all:server client

CC=g++
CXXFLAGS=-I$(INCLUDE) -std=c++17 -O3 -g
INCLUDE=./include

server.o:server.cpp
client.o:client.cpp
tcpServer.o:tcpServer.cpp
threadPool.o:threadPool.cpp
connectionManager.o:connectionManager.cpp
clientConnectionManager.o:clientConnectionManager.cpp
./dispatcher/pollDispatcher.o:./dispatcher/pollDispatcher.cpp


server:server.o tcpServer.o threadPool.o connectionManager.o ./dispatcher/pollDispatcher.o
	$(CC) -o $@ $^ -L. -lpthread -lcrypto -lssl
client:client.o tcpServer.o threadPool.o clientConnectionManager.o ./dispatcher/pollDispatcher.o
	$(CC) -o $@ $^ -L. -lpthread -lcrypto -lssl

clean:
	find . -name '*.o' -type f -print -exec rm -rf {} \;
	rm server
	rm client
