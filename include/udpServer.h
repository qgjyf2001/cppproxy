#ifndef UDPSERVER_H
#define UDPSERVER_H
#include <iostream>
#include <string.h>
#include <map>

#include "threadPool.h"

#define MAXLINE 4096
#define LISTENQ 5

class udpServer
{
private:
    int proxyPort;
    std::string proxyIP;
    int maxClient;
    std::mutex mutex,rwLock;
    std::map<int,int> proxy2actualMap;//真实fd到代理fd的映射
    std::map<int,int> actual2proxyMap;//真实fd到代理fd的映射
    safeQueue<int> actualfds;
public:
    enum serviceType {
        CLIENT,SERVER
    };
    udpServer(int proxyPort=0,std::string proxyIP="0.0.0.0",int maxClient=65536);//代理端口
    void doProxy(safeQueue<std::promise<int>> &connections,serviceType type,int *port=nullptr,std::string *ipAddress=nullptr);//真实端口
    ~udpServer();
};
#endif
