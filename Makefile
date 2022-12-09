all:server client    
windows:client.exe
CC=g++
Type=TCP
ifeq ($(LANG),)
	INCLUDE=-I./include -I"C:/Program Files/OpenSSL-Win64/include"
else
	INCLUDE=-I./include
endif
serverType=TCPSERVER
ifeq ($(Type),UDP)
	serverType=UDPSERVER
endif
CXXFLAGS=$(INCLUDE) -std=c++17 -O3 -g -D$(serverType)
server.o:server.cpp
client.o:client.cpp
tcpServer.o:tcpServer.cpp
udpServer.o:udpServer.cpp
threadPool.o:threadPool.cpp

connectionManager.o:connectionManager.cpp
clientConnectionManager.o:clientConnectionManager.cpp

./dispatcher/pollDispatcher.o:./dispatcher/pollDispatcher.cpp
./dispatcher/winSelectDispatcher.o:./dispatcher/winSelectDispatcher.cpp

./hook/http/httpRequestFilter.o:./hook/http/httpRequestFilter.cpp
./hook/http/httpRequestParser.o:./hook/http/httpRequestParser.cpp
./hook/myFilter1.o:./hook/myFilter1.cpp

#./hook/http/httpHook.a:./hook/http/httpRequestFilter.o ./hook/http/httpRequestParser.o
#	$(AR) cq $@ $^

server:server.o tcpServer.o udpServer.o threadPool.o connectionManager.o ./dispatcher/pollDispatcher.o ./hook/http/httpRequestFilter.o ./hook/http/httpRequestParser.o ./hook/myFilter1.o
	$(CC) -o $@ $^ -L. -lpthread -lcrypto -lssl
client:client.o tcpServer.o udpServer.o threadPool.o clientConnectionManager.o ./dispatcher/pollDispatcher.o ./hook/http/httpRequestFilter.o ./hook/http/httpRequestParser.o
	$(CC) -o $@ $^ -L. -lpthread -lcrypto -lssl
client.exe:client.o tcpServer.o threadPool.o clientConnectionManager.o ./dispatcher/winSelectDispatcher.o
	$(CC) -o $@ $^ -L. -lpthread -L "C:/Program Files/OpenSSL-Win64/lib/mingw/x86" -lcrypto -lssl -lws2_32
clean:
	find . -name '*.o' -type f -print -exec rm -rf {} \;
	find . -name '*.a' -type f -print -exec rm -rf {} \;
	rm client
	rm server
	rm client.exe