#include "clientConnectionManager.h"
#include "md5Helper.h"
#include "../dispatcher/dispatcher.h"

void clientConnectionManager::doManage(std::string token,safeQueue<int>& sockfds,volatile bool *quitPtr) {
    dispatcher* patcher;
#if defined(_WIN32) || defined(_WIN64)
    patcher=new winSelectDispatcher(0);
#else
    patcher=new pollDispatcher(0,false);
#endif
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);//向真实端口发起连接
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
        std::string data="done\n";
        patcher->read(sockfd,buf,MAXLINE);
        if (*quitPtr) {
            data="logout\n";
            patcher->fullWrite(sockfd,data.c_str(),data.length());
            break;
        }
        patcher->fullWrite(sockfd,data.c_str(),data.length());
        int fd = socket(AF_INET, SOCK_STREAM, 0);//向真实端口发起连接
        if(connect(fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        {
            std::cerr<<"connect error at:"<<__FILE__<<",line"<<__LINE__<<std::endl;;
            continue;
        }
        sockfds.push(fd);
        std::cout<<"new connection"<<std::endl;
    }
}