#ifndef POLLDISPATCHER_H
#define POLLDISPATCHER_H
#include "dispatcher.h"
#include <string.h>
#include <iostream>

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

class pollDispatcher : public dispatcher
{
private:
    pollfd* clientfd;
    int totfd=0;
    int listenfd=-1;
public:
    pollDispatcher(int maxClient,bool isListen=false,std::string ip="",int port=0):dispatcher(maxClient) {
        clientfd=new pollfd[maxClient];
        sockaddr_in servaddr;
        for (int i=0;i<maxClient;i++)
        {
            clientfd[i].fd=-1;//ignored
            clientfd[i].events=0;
        }
        if (isListen) {
            listenfd=socket(AF_INET,SOCK_STREAM,0);
            if (listenfd==-1)
                throw std::runtime_error("socket error");
            memset(&servaddr,0,sizeof(servaddr));
            servaddr.sin_family=AF_INET;
            inet_pton(AF_INET,ip.c_str(),&servaddr.sin_addr);
            servaddr.sin_port=htons(port);
            if (bind(listenfd,(sockaddr*)&servaddr,sizeof(servaddr))==-1)
                throw std::runtime_error("bind error");

            //listen
            listen(listenfd,LISTENQ);

            clientfd[0].fd=listenfd;
            clientfd[0].events=POLLIN|POLLOUT;
            clientfd[0].revents=0;
        }
    }
    int connect() override;
    int insert(int fd) override;
    int remove(int index) override;
    int read(int sockfd,char* buf,int size) override {
        return ::read(sockfd,buf,size);
    }
    int write(int sockfd,const char* buf,int size) override {
        return ::write(sockfd,buf,size);
    }
    void doDispatch(std::function<int(int,int)> onRead,std::function<int(int)> onWrite,std::function<void()> onDispatch=nullptr,std::function<int(int)> onConnect=nullptr) override;
    ~pollDispatcher() {
        delete[] clientfd;
    }
};
#endif
