#include "tcpServer.h"
tcpServer::tcpServer(int proxyPort,std::string proxyIP,int maxClient)
{
    signal(SIGPIPE , SIG_IGN);
    this->proxyPort=proxyPort;
    this->proxyIP=proxyIP;
    this->maxClient=maxClient/2;
}
void tcpServer::doProxy(safeQueue<int>& connections,serviceType type,int port,std::string ipAddress)
{
    threadPool pool(4);

    //init
        std::thread proxyThread([&](){
        auto *clientfd=new pollfd[maxClient];
        for (int i=1;i<maxClient;i++)
        {
            clientfd[i].fd=-1;//ignored
            clientfd[i].events=0;
        }

        sockaddr_in servaddr,clientaddr;
        int listenfd;
        if (type==SERVER) {
            listenfd=socket(AF_INET,SOCK_STREAM,0);
            if (listenfd==-1)
                throw std::runtime_error("socket error");
            bzero(&servaddr,sizeof(servaddr));
            servaddr.sin_family=AF_INET;
            inet_pton(AF_INET,proxyIP.c_str(),&servaddr.sin_addr);
            servaddr.sin_port=htons(proxyPort);
            if (bind(listenfd,(sockaddr*)&servaddr,sizeof(servaddr))==-1)
                throw std::runtime_error("bind error");

            //listen
            listen(listenfd,LISTENQ);

            clientfd[0].fd=listenfd;
            clientfd[0].events=POLLIN;
            clientfd[0].revents=0;
        }
        int totfd=0;
        int cnt=0;
        while (true)
        {
            auto nready=poll(clientfd,totfd+1,10);
            if (nready==-1)
                throw std::runtime_error("poll error");
            if (type==SERVER&&(clientfd[0].revents&POLLIN) ||
                type==CLIENT && !connections.empty())
            {
                std::cout<<"new connections:"<<(type==SERVER?"server":"client")<<std::endl;
                socklen_t clientAddrLen=sizeof(clientaddr);
                int connfd;
                if (type==SERVER) {
                    connfd=accept(listenfd,(sockaddr*)&clientaddr,&clientAddrLen);
                    if (connfd==-1)
                        if(errno == EINTR)
                            continue;
                        else
                            throw std::runtime_error("accept error");
                } else {
                    while (!connections.pop(connfd));
                }
                int i;
                for (i=1;i<maxClient;i++)
                    if (clientfd[i].fd<0)
                    {            
                        clientfd[i].events=POLLIN;
                        clientfd[i].fd=connfd;
                        break;
                    }
                if (i==maxClient)//server busy
                {
                    close(connfd);
                    continue;
                }
                totfd=std::max(i,totfd);
                
                int sockfd;
                if (type==SERVER)
                    while (!connections.pop(sockfd));
                else {
                    sockfd=socket(AF_INET, SOCK_STREAM, 0);//向真实端口发起连接
                    bzero(&servaddr, sizeof(servaddr));
                    servaddr.sin_family = AF_INET;
                    servaddr.sin_port = htons(port);
                    inet_pton(AF_INET, ipAddress.c_str(), &servaddr.sin_addr);
            
                    if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
                    {
                        std::cerr<<"connect error"<<std::endl;;
                        continue;
                    }
                }
                if (sockfd==-1) {
                    for (auto &[u,v]:proxy2actualMap) {
                        shutdown(u,SHUT_RDWR);
                        shutdown(v,SHUT_RDWR);
                    }//reset
                    continue;
                }
                proxy2actualMap[connfd]=sockfd;
                actual2proxyMap[sockfd]=connfd;

                actualfds.push(sockfd);
            }
            else
            {
                char buf[MAXLINE];
                for (int i=1;i<maxClient;i++)
                {
                    if (clientfd[i].fd<0)
                        continue;
                    if (clientfd[i].revents&POLLIN)
                    {
                        
                        // std::cout<<"reading...:"<<(type==SERVER?"server":"client")<<std::endl;
                        std::lock_guard<std::mutex> lck(rwLock);
                        auto n=read(clientfd[i].fd,buf,MAXLINE);
                        
                        // std::cout<<"buffer:"<<std::string(buf,n)<<" "<<(type==SERVER?"server":"client")<<std::endl;
                        if (n<=0)
                        {
                            close(clientfd[i].fd);
                            clientfd[i].fd=-1;
                            continue;
                        }   
                        int actualfd=proxy2actualMap[clientfd[i].fd];
                        pool.addThread([this](int actualfd,std::string buf){
                            std::lock_guard<std::mutex> lck(mutex);
                            write(actualfd,buf.c_str(),buf.length());
                        },actualfd,std::string(buf,n));
                    }
                }
            }
        }
    });

    std::thread netThread([&](){
        auto *clientfd=new pollfd[maxClient];
        for (int i=1;i<maxClient;i++)
        {
            clientfd[i].fd=-1;//ignored
            clientfd[i].events=0;
        }
        int totfd=0;
        int cnt=0;
        while (true)
        {
            if (totfd>0)
            {
                auto nready=poll(clientfd,totfd,10);
                if (nready==-1)
                    throw std::runtime_error("poll error");
            }
            if (!actualfds.empty())
            {
                int connfd,i;
                actualfds.pop(connfd);
                for (i=0;i<maxClient;i++)
                    if (clientfd[i].fd<0)
                    {            
                        clientfd[i].events=POLLIN;
                        clientfd[i].fd=connfd;
                        break;
                    }
                if (i==maxClient)//server busy
                {
                    close(connfd);
                    continue;
                }
                totfd=std::max(i+1,totfd);
            }
            else
            {
                char buf[MAXLINE];
                for (int i=0;i<maxClient;i++)
                {
                    if (clientfd[i].fd<0)
                        continue;
                    if (clientfd[i].revents&POLLIN)
                    {
                        std::lock_guard<std::mutex> lck(rwLock);
                        auto n=read(clientfd[i].fd,buf,MAXLINE);
                        if (n<=0)
                        {
                            close(clientfd[i].fd);
                            clientfd[i].fd=-1;
                            continue;
                        }   
                        int proxy=actual2proxyMap[clientfd[i].fd];
                        pool.addThread([this](int proxy,std::string buf){
                            std::lock_guard<std::mutex> lck(mutex);
                            write(proxy,buf.c_str(),buf.length());
                        },proxy,std::string(buf,n));
                    }
                }
            }
        }
    });
    proxyThread.join();
    netThread.join();
}
tcpServer::~tcpServer()
{
}
