#include "clientConnectionManager.h"
#include "md5Helper.h"
#include "../dispatcher/dispatcher.h"

#if defined(_WIN32) || defined(_WIN64)
#else
#include <netinet/tcp.h>
#endif

void clientConnectionManager::doManage(std::string token,safeQueue<std::promise<int>>& sockfds,volatile bool *quitPtr) {
    reconnect:
    dispatcher* patcher;
#if defined(_WIN32) || defined(_WIN64)
    auto patcher_=new winSelectDispatcher();
    patcher_->init(0);
    patcher=patcher_;
#else
    auto patcher_=new pollDispatcher();
    patcher_->init(0,false);
    patcher=patcher_;
#endif

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);//向真实端口发起连接

#if defined(_WIN32) || defined(_WIN64)
#else
    int keepIdle = 6;     /*开始首次KeepAlive探测前的TCP空闭时间 */
    int keepInterval = 5; /* 两次KeepAlive探测间的时间间隔  */
    int keepCount = 3;    /* 判定断开前的KeepAlive探测次数 */
    int keepalive = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive , sizeof(keepalive ));
    setsockopt(sockfd, SOL_TCP, TCP_KEEPIDLE, (void *)&keepIdle, sizeof(keepIdle));
    setsockopt(sockfd, SOL_TCP,TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval));
    setsockopt(sockfd,SOL_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount)); 
#endif

    sockaddr_in servaddr;
    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);

#if defined(_WIN32) || defined(_WIN64)
    servaddr.sin_addr.S_un.S_addr = inet_addr(ip.c_str());	
#else
    inet_pton(AF_INET, ip.c_str(), &servaddr.sin_addr);
#endif
        
    if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        std::cerr<<"connect error at:"<<__FILE__<<",line"<<__LINE__<<std::endl;;
    }
    char buf[MAXLINE];
    auto nb=patcher->read(sockfd,buf,MAXLINE);
    buf[nb]=0;
    std::string rnd=std::to_string(std::atoi(buf));
    std::cout<<"challlenge:"<<rnd<<std::endl;
    std::string buffer=md5Helper::md5(token+rnd)+"\n";
    patcher->fullWrite(sockfd,buffer.c_str(),buffer.length());
    auto n=patcher->read(sockfd,buf,MAXLINE);
    std::string reply=std::string(buf,n);
    if (reply.substr(0,7)!="success") {
        throw std::runtime_error("authentication failed");
    }
    std::cout<<"login success"<<std::endl;
    patcher->fullWrite(sockfd,"ready\n",6);
    while (true)
    {
        std::string data=std::to_string(rand())+"\n";
        bool status;
        int n=patcher->read(sockfd,buf,MAXLINE);
        if (n<0) {
            std::cout<<"read failed with error code "<<n<<std::endl;
            goto brokenPipe;
        }
        if (*quitPtr) {
            data="logout\n";
            patcher->fullWrite(sockfd,data.c_str(),data.length());
            break;
        }
        status=patcher->fullWrite(sockfd,data.c_str(),data.length());
        if (!status) {
            std::cout<<"write failed"<<std::endl;
            brokenPipe:
#if defined(_WIN32) || defined(_WIN64)
                closesocket(sockfd);
#else
                close(sockfd);
#endif
            goto reconnect;
        }
        int fd = socket(AF_INET, SOCK_STREAM, 0);//向真实端口发起连接
        if(connect(fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        {
            std::cerr<<"connect error at:"<<__FILE__<<",line"<<__LINE__<<std::endl;;
            continue;
        }
        patcher->fullWrite(fd,data.c_str(),data.length());
        std::promise<int> promise;
        promise.set_value(fd);
        sockfds.push(std::move(promise));
        std::cout<<"new connection"<<std::endl;
    }
}
