#include "tcpServer.h"
#include "clientConnectionManager.h"
std::string remoteIP="127.0.0.1";
int remotePort=8081;
int main() {
    safeQueue<int> connections;
    std::thread manageThread([&](){
        clientConnectionManager manager(remoteIP,remotePort);
        manager.doManage("123456",connections);
    });
    tcpServer server;
    server.doProxy(connections,tcpServer::CLIENT,22,"127.0.0.1");
    manageThread.join();
}