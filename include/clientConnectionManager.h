#ifndef CLIENTCONNECTIONMANAGER_H
#define CLIENTCONNECTIONMANAGER_H

#include <iostream>
#include "safeQueue.h"
#include <string.h>

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

class clientConnectionManager
{
private:
    std::string ip;
    int port;
    int maxClient;
public:
    clientConnectionManager(std::string ip,int port,int maxClient=65536):port(port),ip(ip),maxClient(maxClient) {

    }
    void doManage(std::string token,safeQueue<int>& sockfds);
};

#endif