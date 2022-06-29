all:server client
windows:client.exe
CC=g++
CXXFLAGS=$(INCLUDE) -std=c++17 -O3 -g
ifeq ($(LANG),)
	INCLUDE=-I./include -I"C:/Program Files/OpenSSL-Win64/include"
else
	INCLUDE=-I./include
endif

server.o:server.cpp
client.o:client.cpp
tcpServer.o:tcpServer.cpp
threadPool.o:threadPool.cpp
connectionManager.o:connectionManager.cpp
clientConnectionManager.o:clientConnectionManager.cpp
./dispatcher/pollDispatcher.o:./dispatcher/pollDispatcher.cpp
./dispatcher/winSelectDispatcher.o:./dispatcher/winSelectDispatcher.cpp

server:server.o tcpServer.o threadPool.o connectionManager.o ./dispatcher/pollDispatcher.o
	$(CC) -o $@ $^ -L. -lpthread -lcrypto -lssl
client:client.o tcpServer.o threadPool.o clientConnectionManager.o ./dispatcher/pollDispatcher.o
	$(CC) -o $@ $^ -L. -lpthread -lcrypto -lssl
client.exe:client.o tcpServer.o threadPool.o clientConnectionManager.o ./dispatcher/winSelectDispatcher.o
	$(CC) -o $@ $^ -L. -lpthread -L "C:/Program Files/OpenSSL-Win64/lib/mingw/x86" -lcrypto -lssl -lws2_32
clean:
	find . -name '*.o' -type f -print -exec rm -rf {} \;
	rm client
	rm server
	rm client.exe