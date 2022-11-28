#include "connectionManager.h"
#include "md5Helper.h"
#include "../dispatcher/dispatcher.h"

void connectionManager::doManage(std::string token,safeQueue<int>& sockfds) {
    auto *patcher=new pollDispatcher(0,false);

    srand(time(NULL));
//init
    sockaddr_in servaddr,clientaddr;
    auto listenfd=socket(AF_INET,SOCK_STREAM,0);
    if (listenfd==-1)
        throw std::runtime_error("socket error");
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family=AF_INET;
    inet_pton(AF_INET,ip.c_str(),&servaddr.sin_addr);
    servaddr.sin_port=htons(port);
    if (bind(listenfd,(sockaddr*)&servaddr,sizeof(servaddr))==-1)
        throw std::runtime_error("bind error");

    //listen
    listen(listenfd,LISTENQ);
    
    int connfd;
    socklen_t clientAddrLen=sizeof(clientaddr);
    while (true) {
        while (true) {
            std::cout<<"listening"<<std::endl;
            connfd=accept(listenfd,(sockaddr*)&clientaddr,&clientAddrLen);
            if (connfd==-1)
                if(errno == EINTR)
                    continue;
                else
                    throw std::runtime_error("accept error");
            char buf[MAXLINE]={0};
            std::string rnd=std::to_string(rand());
            std::cout<<"challlenge:"<<rnd<<std::endl;
            patcher->fullWrite(connfd,(rnd+"\n").c_str(),rnd.length());
            auto n=read(connfd,buf,MAXLINE);
            std::string buffer(buf,n);
            std::string result="failed\n";
            std::string ans=md5Helper::md5(token+rnd);
            if (buffer.substr(0,ans.length())==ans) {
                result="success\n";
                patcher->fullWrite(connfd,result.c_str(),result.length());
                break;
            }
            close(connfd);
        }
        char buf[7];
        read(connfd,buf,7);
        while (true) {
            std::string data="CONNECT\n";
            char buffer[MAXLINE];
            if (sockfds.empty()) {
                patcher->fullWrite(connfd,data.c_str(),data.length());
                auto n=read(connfd,buffer,MAXLINE);
                std::string res(buffer,n);
                std::string logout="logout";
                if (res.substr(0,logout.length())==logout) {
                    std::cout<<"logout"<<std::endl;
                    sockfds.push(-1);
                    break;
                }
                int fd=accept(listenfd,(sockaddr*)&clientaddr,&clientAddrLen);
                sockfds.push(fd);
            }
        }
    }
    
}
