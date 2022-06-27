#include "clientConnectionManager.h"
#include "md5Helper.h"

void clientConnectionManager::doManage(std::string token,safeQueue<int>& sockfds) {
    
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);//向真实端口发起连接
    sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &servaddr.sin_addr);
        
    if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        std::cerr<<"connect error at:"<<__FILE__<<",line"<<__LINE__<<std::endl;;
    }
    char buf[MAXLINE];
    auto nb=read(sockfd,buf,MAXLINE);
    buf[nb]=0;
    std::string rnd=std::to_string(std::atoi(buf));
    std::cout<<"challlenge:"<<rnd<<std::endl;
    std::string buffer=md5Helper::md5(token+rnd)+"\n";
    write(sockfd,buffer.c_str(),buffer.length());
    auto n=read(sockfd,buf,MAXLINE);
    std::string reply=std::string(buf,n);
    if (reply.substr(0,7)!="success") {
        throw std::runtime_error("authentication failed");
    }
    std::cout<<"login success"<<std::endl;
    while (true)
    {
        std::string data="done\n";
        read(sockfd,buf,MAXLINE);
        write(sockfd,data.c_str(),data.length());
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