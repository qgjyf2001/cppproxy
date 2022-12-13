#ifndef WINSELECTDISPATCHER_H
#define WINSELECTDISPATCHER_H
#include "dispatcher.h"
#include <iostream>
#include <WinSock2.h>
#include <string.h>

#define MAXLINE 4096
#define LISTENQ 5

class winSelectDispatcher : public dispatcher
{
private:
    fd_set clientfd;
    fd_set* tmpfd;
public:
    winSelectDispatcher(int maxClient):dispatcher(maxClient) {
        
    }
    int connect() override {//not supported
        return 0;
    }
    int insert(int fd) override;
    virtual int remove(int index) override;
    virtual int read(int sockfd,char* buf,int size) override {
        return ::recv(sockfd,buf,size,0);
    }
    virtual int write(int sockfd,const char* buf,int size) override {
        return ::send(sockfd,buf,size,0);
    }
    void doDispatch(std::function<int(int,int)> onRead,std::function<int(int)> onWrite,std::function<void()> onDispatch=nullptr,std::function<int(int)> onConnect=nullptr) override;
    ~winSelectDispatcher() {
    }
};

#endif