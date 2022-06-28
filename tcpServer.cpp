#include "tcpServer.h"
#include "../dispatcher/dispatcher.h"
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
        dispatcher* patcher;
        char buf[MAXLINE];
        auto onRead=[&](int index,int sockfd){
            std::lock_guard<std::mutex> lck(rwLock);
            auto n=patcher->read(sockfd,buf,MAXLINE);
            if (n<=0)
            {
                patcher->remove(index);
                return 0;
            }   
            int actualfd=proxy2actualMap[sockfd];
            pool.addThread([&](int actualfd,std::string buf){
                std::lock_guard<std::mutex> lck(mutex);
                patcher->write(actualfd,buf.c_str(),buf.length());
            },actualfd,std::string(buf,n));
            return 0;
        };
        if (type==SERVER)
        {
            patcher=new pollDispatcher(maxClient,true,proxyIP,proxyPort);
            auto onConnect=[&](int connfd){
                std::cout<<"new connections:"<<(type==SERVER?"server":"client")<<std::endl;
                sockaddr_in servaddr;
                int sockfd;
                while (!connections.pop(sockfd));
                if (sockfd==-1) {
                    for (auto &[u,v]:proxy2actualMap) {
                        shutdown(u,SHUT_RDWR);
                        shutdown(v,SHUT_RDWR);
                    }//reset
                    return 0;
                }
                proxy2actualMap[connfd]=sockfd;
                actual2proxyMap[sockfd]=connfd;

                actualfds.push(sockfd);
                return 0;
            };
            patcher->doDispatch(onRead,nullptr,onConnect);
        } else {
            patcher=new pollDispatcher(maxClient,false);
            auto onDispatch=[&](){
                if (!connections.empty()) {
                    sockaddr_in servaddr;
                    std::cout<<"new connections:"<<(type==SERVER?"server":"client")<<std::endl;
                    int connfd;
                    while (!connections.pop(connfd));
                    patcher->insert(connfd);
                    
                    int sockfd=socket(AF_INET, SOCK_STREAM, 0);//向真实端口发起连接
                    bzero(&servaddr, sizeof(servaddr));
                    servaddr.sin_family = AF_INET;
                    servaddr.sin_port = htons(port);
                    inet_pton(AF_INET, ipAddress.c_str(), &servaddr.sin_addr);
                
                    if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
                    {
                        std::cerr<<"connect error"<<std::endl;;
                        return;
                    }
                    
                    if (sockfd==-1) {
                        for (auto &[u,v]:proxy2actualMap) {
                            shutdown(u,SHUT_RDWR);
                            shutdown(v,SHUT_RDWR);
                        }//reset
                        return;
                    }
                    proxy2actualMap[connfd]=sockfd;
                    actual2proxyMap[sockfd]=connfd;

                    actualfds.push(sockfd);
                }
                return;
            };
            patcher->doDispatch(onRead,onDispatch,nullptr);
        }
    });

    std::thread netThread([&](){
        dispatcher* patcher;
        patcher=new pollDispatcher(maxClient,false);
        char buf[MAXLINE];
        auto onRead=[&](int index,int sockfd) {
            std::lock_guard<std::mutex> lck(rwLock);
            auto n=patcher->read(sockfd,buf,MAXLINE);
            if (n<=0)
            {
                patcher->remove(index);
                return 0;
            }   
            int proxy=actual2proxyMap[sockfd];
            pool.addThread([&](int proxy,std::string buf){
                std::lock_guard<std::mutex> lck(mutex);
                patcher->write(proxy,buf.c_str(),buf.length());
            },proxy,std::string(buf,n));
            return 0;
        };
        auto onDispatch=[&](){
            if (!actualfds.empty())
            {
                int connfd,i;
                actualfds.pop(connfd);
                patcher->insert(connfd);
            }
            return;
        };
        patcher->doDispatch(onRead,onDispatch,nullptr);
    });
    proxyThread.join();
    netThread.join();
}
tcpServer::~tcpServer()
{
}
