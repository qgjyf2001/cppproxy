#include "tcpServer.h"
#include "dispatcher/dispatcher.h"
std::map<int,int> tcpServer::actual2proxyMap;
tcpServer::tcpServer(int proxyPort,std::string proxyIP,int maxClient)
{
#if !(defined(_WIN32) || defined(_WIN64))
    signal(SIGPIPE , SIG_IGN);
#endif
    this->proxyPort=proxyPort;
    this->proxyIP=proxyIP;
    this->maxClient=maxClient/2;
}
void tcpServer::doProxy(safeQueue<std::promise<int>> &connections,serviceType type,int* port,std::string* ipAddress)
{
    std::vector<std::pair<safeQueue<std::string>,int>> proxyWriteBuffer(maxClient);
    std::vector<std::pair<safeQueue<std::string>,int>> netWriteBuffer(maxClient);
    bool proxyRemovedFd[maxClient]={0};
    bool netRemovedFd[maxClient]={0};
    safeQueue<int> proxyFastRemovedFd,netFastRemovedFd;
    //init
    std::shared_ptr<dispatcher> proxyPatcher,netPatcher;
        std::thread proxyThread([&](){
        char buf[MAXLINE];
        auto onWrite=[&](int sockfd) {
            if (proxyWriteBuffer[sockfd].first.empty()) {
                return 0;
            }
            auto& buffer=proxyWriteBuffer[sockfd].first.front();
            int &offset=proxyWriteBuffer[sockfd].second;
            auto *data=buffer.data()+offset;
            int nsize=proxyPatcher->write(sockfd,data,buffer.length()-offset);
            offset+=nsize;
            if (offset==buffer.length()) {
                std::string _;
                proxyWriteBuffer[sockfd].first.pop(_);
                offset=0;
                if (proxyRemovedFd[sockfd]&& proxyWriteBuffer[sockfd].first.empty()) {
                    proxyPatcher->remove(sockfd);
                    proxyRemovedFd[sockfd]=false;
                }
            }
            return 0;
        };
        auto onRead=[&](int index,int sockfd){
            auto n=proxyPatcher->read(sockfd,buf,MAXLINE);
            int actualfd=proxy2actualMap[sockfd];
            if (n<=0)
            {
                proxyPatcher->remove(sockfd);
                if (netWriteBuffer[actualfd].first.empty()) {
                    netFastRemovedFd.push(actualfd);
                } else {
                    netRemovedFd[actualfd]=true;
                }
                return 0;
            }   
            netWriteBuffer[actualfd].first.push(std::string(buf,n));
            return 0;
        };
        
        auto clientConfig=config::instance().json["clientDispatcher"];
        proxyPatcher=dispatcher::get(clientConfig);
        
        if (type==SERVER)
        {
            auto onConnect=[&](int connfd){
                std::cout<<"new connections:"<<(type==SERVER?"server":"client")<<std::endl;
                sockaddr_in servaddr;
                std::promise<int> promise;
                auto sockfd_=promise.get_future();
                connections.push(std::move(promise));
                int sockfd=sockfd_.get();
                if (sockfd==-1) {
                    for (auto &[u,v]:proxy2actualMap) {
#if defined(_WIN32) || defined(_WIN64)
                            shutdown(u,SD_BOTH);
                            shutdown(v,SD_BOTH);
#else
                            shutdown(u,SHUT_RDWR);
                            shutdown(v,SHUT_RDWR);
#endif
                    }//reset
                    return 0;
                }
                proxy2actualMap[connfd]=sockfd;
                actual2proxyMap[sockfd]=connfd;

                actualfds.push(sockfd);
                return 0;
            };
            auto onDispatch=[&](){
                int sockfd_;
                while (proxyFastRemovedFd.pop(sockfd_)) {
                    proxyPatcher->remove(sockfd_);
                }
            };
            proxyPatcher->doDispatch(onRead,onWrite,onDispatch,onConnect);
        } else {
            auto onDispatch=[&](){
                int sockfd_;
                while (proxyFastRemovedFd.pop(sockfd_)) {
                    proxyPatcher->remove(sockfd_);
                }
                if (!connections.empty()) {
                    sockaddr_in servaddr;
                    std::cout<<"new connections:"<<(type==SERVER?"server":"client")<<std::endl;
                    std::promise<int> sockfd_;
                    connections.pop(sockfd_);
                    int sockfd=sockfd_.get_future().get();
                    
                    int connfd=socket(AF_INET, SOCK_STREAM, 0);//向真实端口发起连接
                    memset(&servaddr,0, sizeof(servaddr));
                    servaddr.sin_family = AF_INET;
                    servaddr.sin_port = htons(*port);
#if defined(_WIN32) || defined(_WIN64)
                    servaddr.sin_addr.S_un.S_addr = inet_addr(ipAddress->c_str());
#else
                    inet_pton(AF_INET, ipAddress->c_str(), &servaddr.sin_addr);
#endif
                
                    if(connect(connfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
                    {
                        std::cerr<<"connect error"<<std::endl;;
                        return;
                    }
                    
                    if (connfd==-1) {
                        for (auto &[u,v]:proxy2actualMap) {
#if defined(_WIN32) || defined(_WIN64)
                            shutdown(u,SD_BOTH);
                            shutdown(v,SD_BOTH);
#else
                            shutdown(u,SHUT_RDWR);
                            shutdown(v,SHUT_RDWR);
#endif
                        }//reset
                        for (int i=0;i<maxClient;i++) {
                            proxyWriteBuffer[i].first.clear();
                            netWriteBuffer[i].first.clear();
                            proxyWriteBuffer[i].second=netWriteBuffer[i].second=0;
                        }
                        return;
                    }
                    proxy2actualMap[connfd]=sockfd;
                    actual2proxyMap[sockfd]=connfd;

                    actualfds.push(sockfd);
                    proxyPatcher->insert(connfd);
                }
                return;
            };
            proxyPatcher->doDispatch(onRead,onWrite,onDispatch,nullptr);
        }
    });

    std::thread netThread([&](){
        auto proxyConfig=config::instance().json["proxyDispatcher"];
        netPatcher=dispatcher::get(proxyConfig);
        char buf[MAXLINE];
        
        auto onWrite=[&](int sockfd) {
            if (netWriteBuffer[sockfd].first.empty()) {
                return 0;
            }
            auto& buffer=netWriteBuffer[sockfd].first.front();
            int &offset=netWriteBuffer[sockfd].second;
            auto *data=buffer.data()+offset;
            int nsize=netPatcher->write(sockfd,data,buffer.length()-offset);
            offset+=nsize;
            if (offset==buffer.length()) {
                std::string _;
                netWriteBuffer[sockfd].first.pop(_);
                offset=0;
                if (netRemovedFd[sockfd]&& netWriteBuffer[sockfd].first.empty()) {
                    netPatcher->remove(sockfd);
                    netRemovedFd[sockfd]=false;
                }
            }
            return 0;
        };
        auto onRead=[&](int index,int sockfd) {
            auto n=netPatcher->read(sockfd,buf,MAXLINE);
            if (n==0) {
                return 0;
            }
            int proxyfd=actual2proxyMap[sockfd];
            if (n<0)
            {
                
                netPatcher->remove(sockfd);
                if (proxyWriteBuffer[proxyfd].first.empty()) {
                    proxyFastRemovedFd.push(proxyfd);
                } else {
                    proxyRemovedFd[proxyfd]=true;
                }
                return 0;
            }   
            proxyWriteBuffer[proxyfd].first.push(std::string(buf,n));
            
            return 0;
        };
        auto onDispatch=[&](){
            int sockfd;
            while (netFastRemovedFd.pop(sockfd)) {
                netPatcher->remove(sockfd);
            }
            if (!actualfds.empty())
            {
                int connfd,i;
                actualfds.pop(connfd);
                netPatcher->insert(connfd);
            }
            return;
        };
        netPatcher->doDispatch(onRead,onWrite,onDispatch,nullptr);
    });
    proxyThread.join();
    netThread.join();
}
tcpServer::~tcpServer()
{
}