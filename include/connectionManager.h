#ifndef CONNECTIONMANAGER_H
#define CONNECTIONMANAGER_H

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

class connectionManager
{
private:
    std::string ip;
    int port;
    int maxClient;
public:
    connectionManager(int port,std::string ip="0.0.0.0",int maxClient=65536):port(port),ip(ip),maxClient(maxClient) {

    }
    void doManage(std::string token,safeQueue<int>& sockfds);
};

#endif