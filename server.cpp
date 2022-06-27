#include "tcpServer.h"
#include "connectionManager.h"
int main() {
    safeQueue<int> connections;
    std::thread manageThread([&](){
        connectionManager manager(8081);
        manager.doManage("123456",connections);
    });
    tcpServer server(8000);
    server.doProxy(connections,tcpServer::SERVER);
    manageThread.join();
}