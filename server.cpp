#ifdef TCPSERVER
#include "tcpServer.h"
#else
#include "udpServer.h"
#endif
#include "connectionManager.h"
int main() {
    safeQueue<std::promise<int>> connections;
    std::thread manageThread([&](){
        connectionManager manager(8081);
        manager.doManage("123456",connections);
    });
#ifdef TCPSERVER
    tcpServer server(8000);
    server.doProxy(connections,tcpServer::SERVER);
#else
    udpServer server(8000);
    server.doProxy(connections,udpServer::SERVER);
#endif
    manageThread.join();
}
