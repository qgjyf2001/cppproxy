#ifdef TCPSERVER
#include "tcpServer.h"
#else
#include "udpServer.h"
#endif
#include "connectionManager.h"
#include "config.h"
int main() {
    config::instance().init("./config/server.json");
    int remotePort=config::instance().json["remotePort"].toInt();
    int remoteProxyPort=config::instance().json["remoteProxyPort"].toInt();

    safeQueue<std::promise<int>> connections;
    std::thread manageThread([&](){
        connectionManager manager(remotePort);
        manager.doManage(config::instance().json["password"].toString(),connections);
    });
#ifdef TCPSERVER
    tcpServer server(remoteProxyPort);
    server.doProxy(connections,tcpServer::SERVER);
#else
    udpServer server(remoteProxyPort);
    server.doProxy(connections,udpServer::SERVER);
#endif
    manageThread.join();
}
