#ifndef CLIENTCONNECTIONMANAGER_H
#define CLIENTCONNECTIONMANAGER_H

#include <iostream>
#include "safeQueue.h"
#include "../dispatcher/dispatcher.h"
#include "future"

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
    void doManage(std::string token,safeQueue<std::promise<int>>& sockfds,volatile bool *quitPtr);
};

#endif