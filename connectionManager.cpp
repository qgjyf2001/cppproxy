#include "connectionManager.h"
#include "md5Helper.h"
#include "../dispatcher/dispatcher.h"
#include <netinet/tcp.h>

void connectionManager::doManage(std::string token,safeQueue<std::promise<int>>& sockfds) {
    auto *patcher=new pollDispatcher();
    patcher->init(0,false);

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
    reconnect:
    while (true) {
        while (true) {
            std::cout<<"listening"<<std::endl;
            connfd=accept(listenfd,(sockaddr*)&clientaddr,&clientAddrLen);

            int keepIdle = 6;     /*开始首次KeepAlive探测前的TCP空闭时间 */
            int keepInterval = 5; /* 两次KeepAlive探测间的时间间隔  */
            int keepCount = 3;    /* 判定断开前的KeepAlive探测次数 */
            int keepalive = 1;
            setsockopt(connfd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive , sizeof(keepalive ));
            setsockopt(connfd, SOL_TCP, TCP_KEEPIDLE, (void *)&keepIdle, sizeof(keepIdle));
            setsockopt(connfd, SOL_TCP,TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval));
            setsockopt(connfd,SOL_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount)); 
            
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
            std::promise<int> promise;
            {
                sockfds.sync_pop(promise);
                int n;
                bool status=patcher->fullWrite(connfd,data.c_str(),data.length());
                if (!status) {
                    std::cout<<"write failed"<<std::endl;
                    goto brokenPipe;
                }
                n=read(connfd,buffer,MAXLINE);
                if (n<0) {
                    std::cout<<"read failed with error code "<<n<<std::endl;
                    brokenPipe:
                    close(connfd);
                    sockfds.push(std::move(promise));
                    goto reconnect;
                }
                std::string res(buffer,n);
                std::string logout="logout";
                if (res.substr(0,logout.length())==logout) {
                    std::cout<<"logout"<<std::endl;
                    promise.set_value(-1);
                    break;
                }
                int fd=accept(listenfd,(sockaddr*)&clientaddr,&clientAddrLen);
                n=patcher->read(fd,buffer,MAXLINE);
                if (std::string(buffer,n) != res) {
                    promise.set_value(-1);
                    break;
                }
                promise.set_value(fd);
            }
        }
    }
    
}
