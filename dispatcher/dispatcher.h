#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <functional>
class dispatcher
{
protected:
    int maxClient;
public:
    dispatcher(int maxClient):maxClient(maxClient) {

    }
    virtual int connect()=0;
    virtual int insert(int fd)=0;
    virtual int remove(int index)=0;
    virtual int read(int sockfd,char* buf,int size)=0;
    virtual int write(int sockfd,const char* buf,int size)=0;
    virtual void doDispatch(std::function<int(int,int)> onRead,std::function<void()> onDispatch=nullptr,std::function<int(int)> onConnect=nullptr)=0;//index,sockfd
    
};
#if defined(_WIN32) || defined(_WIN64)
#include "winSelectDispatcher.h"
#else
#include "pollDispatcher.h"
#endif
#endif