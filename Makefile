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


%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

linux_core = tcpServer.o udpServer.o threadPool.o ./dispatcher/pollDispatcher.o connectionManager.o clientConnectionManager.o
windows_core = tcpServer.o threadPool.o ./dispatcher/winSelectDispatcher.o clientConnectionManager.o

http_filter = ./hook/http/httpRequestFilter.o ./hook/http/httpRequestParser.o
https_filter = ./hook/https/httpsClientFilter.o ./hook/https/httpsProxyFilter.o ./hook/https/SSLManager.o
filter_core = $(http_filter) $(https_filter)

udf_filter = ./hook/myFilter1.o

server:server.o $(linux_core) $(filter_core) $(udf_filter)
	$(CC) -o $@ $^ -L. -lpthread -lcrypto -lssl
client:client.o $(linux_core)
	$(CC) -o $@ $^ -L. -lpthread -lcrypto -lssl
client.exe:client.o $(windows_core)
	$(CC) -o $@ $^ -L. -lpthread -L "C:/Program Files/OpenSSL-Win64/lib/mingw/x86" -lcrypto -lssl -lws2_32
clean:
	find . -name '*.o' -type f -print -exec rm -rf {} \;
	find . -name '*.a' -type f -print -exec rm -rf {} \;
	rm client
	rm server
	rm client.exe