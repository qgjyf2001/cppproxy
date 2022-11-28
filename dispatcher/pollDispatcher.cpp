#include "pollDispatcher.h"
int pollDispatcher::insert(int fd) {
    int i;
    for (i=1;i<maxClient;i++)
        if (clientfd[i].fd<0)
        {            
            clientfd[i].events=POLLIN|POLLOUT;
            clientfd[i].fd=fd;
            break;
        }
    if (i==maxClient)//server busy
    {
        close(fd);
        return 0;
    }
    totfd=std::max(i,totfd);
    return totfd;
}
void pollDispatcher::doDispatch(std::function<int(int,int)> onRead,std::function<int(int)> onWrite,std::function<void()> onDispatch,std::function<int(int)> onConnect) {
    while (true)
    {
        auto nready=poll(clientfd,totfd+1,10);
        if (onDispatch!=nullptr) {
            onDispatch();
        }
        if (nready==-1)
            throw std::runtime_error("poll error");
        char buf[MAXLINE];
        for (int i=0;i<maxClient;i++)
        {
            if (clientfd[i].fd<0)
                continue;
            if (clientfd[i].revents&POLLIN)
            {
                if (i==0&&listenfd!=-1) {
                    int connfd=connect();
                    if (onConnect!=nullptr) {
                        onConnect(connfd);
                    }
                }
                else if (i!=0) {
                    onRead(i,clientfd[i].fd);
                }
            }
            if (clientfd[i].revents&POLLOUT)
            {
                onWrite(clientfd[i].fd);
            }
        }
    }
}
int pollDispatcher::connect() {
    sockaddr_in clientaddr;
    socklen_t clientAddrLen=sizeof(clientaddr);
    auto connfd=accept(listenfd,(sockaddr*)&clientaddr,&clientAddrLen);
    if (connfd==-1)
        if(errno == EINTR)
            return -1;
        else
            throw std::runtime_error("accept error");
    insert(connfd);
    return connfd;
}
int pollDispatcher::remove(int index) {
    close(clientfd[index].fd);
    clientfd[index].fd=-1;
    return 0;
}
