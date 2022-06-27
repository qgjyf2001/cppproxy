#ifndef TCPSERVER_H
#define TCPSERVER_H
#include <iostream>
#include <string.h>
#include <map>

#include "threadPool.h"

#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/epoll.h>


#define MAXLINE 4096
#define LISTENQ 5

class tcpServer
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
    tcpServer(int proxyPort=0,std::string proxyIP="0.0.0.0",int maxClient=65536);//代理端口
    void doProxy(safeQueue<int> &connections,serviceType type,int port=0,std::string ipAddress="");//真实端口
    ~tcpServer();
};
#endif