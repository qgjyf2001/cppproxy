#include "tcpServer.h"
#include "dispatcher/dispatcher.h"
tcpServer::tcpServer(int proxyPort,std::string proxyIP,int maxClient)
{
#if !(defined(_WIN32) || defined(_WIN64))
    signal(SIGPIPE , SIG_IGN);
#endif
    this->proxyPort=proxyPort;
    this->proxyIP=proxyIP;
    this->maxClient=maxClient/2;
}
void tcpServer::doProxy(safeQueue<int>& connections,serviceType type,int* port,std::string* ipAddress)
{
    std::map<int,std::pair<std::deque<std::string>,int>> proxyWriteBuffer,netWriteBuffer;
    //init
        std::thread proxyThread([&](){
        dispatcher* patcher;
        char buf[MAXLINE];
        auto onWrite=[&](int sockfd) {
            std::lock_guard<std::mutex> lck(rwLock);
            if (proxyWriteBuffer[sockfd].first.empty()) {
                return 0;
            }
            auto& buffer=proxyWriteBuffer[sockfd].first.front();
            int &offset=proxyWriteBuffer[sockfd].second;
            auto *data=buffer.data()+offset;
            int nsize=patcher->write(sockfd,data,buffer.length()-offset);
            offset+=nsize;
            if (offset==buffer.length()) {
                proxyWriteBuffer[sockfd].first.pop_front();
                offset=0;
            }
            return 0;
        };
        auto onRead=[&](int index,int sockfd){
            std::lock_guard<std::mutex> lck(rwLock);
            auto n=patcher->read(sockfd,buf,MAXLINE);
            if (n<=0)
            {
                patcher->remove(index);
                return 0;
            }   
            int actualfd=proxy2actualMap[sockfd];
            netWriteBuffer[actualfd].first.push_back(std::string(buf,n));
            return 0;
        };
        if (type==SERVER)
        {

#if defined(_WIN32) || defined(_WIN64)
#else
            patcher=new pollDispatcher(maxClient,true,proxyIP,proxyPort);
#endif
            auto onConnect=[&](int connfd){
                std::cout<<"new connections:"<<(type==SERVER?"server":"client")<<std::endl;
                sockaddr_in servaddr;
                int sockfd;
                connections.pop(sockfd);//-1
                while (connections.empty());
                connections.push(-1);
                connections.pop(sockfd);
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
            patcher->doDispatch(onRead,onWrite,nullptr,onConnect);
        } else {

#if defined(_WIN32) || defined(_WIN64)
            patcher=new winSelectDispatcher(1234);
#else
            patcher=new pollDispatcher(maxClient,false);
#endif
            auto onDispatch=[&](){
                if (!connections.empty()) {
                    sockaddr_in servaddr;
                    std::cout<<"new connections:"<<(type==SERVER?"server":"client")<<std::endl;
                    int connfd;
                    while (!connections.pop(connfd));
                    patcher->insert(connfd);
                    
                    int sockfd=socket(AF_INET, SOCK_STREAM, 0);//???????????????????????????
                    memset(&servaddr,0, sizeof(servaddr));
                    servaddr.sin_family = AF_INET;
                    servaddr.sin_port = htons(*port);
#if defined(_WIN32) || defined(_WIN64)
                    servaddr.sin_addr.S_un.S_addr = inet_addr(ipAddress->c_str());
#else
                    inet_pton(AF_INET, ipAddress->c_str(), &servaddr.sin_addr);
#endif
                
                    if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
                    {
                        std::cerr<<"connect error"<<std::endl;;
                        return;
                    }
                    
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
                        proxyWriteBuffer.clear();
                        netWriteBuffer.clear();
                        return;
                    }
                    proxy2actualMap[connfd]=sockfd;
                    actual2proxyMap[sockfd]=connfd;

                    actualfds.push(sockfd);
                }
                return;
            };
            patcher->doDispatch(onRead,onWrite,onDispatch,nullptr);
        }
    });

    std::thread netThread([&](){
        dispatcher* patcher;

#if defined(_WIN32) || defined(_WIN64)
        patcher=new winSelectDispatcher(maxClient);
#else
        patcher=new pollDispatcher(maxClient,false);
#endif
        char buf[MAXLINE];
        
        auto onWrite=[&](int sockfd) {
            std::lock_guard<std::mutex> lck(rwLock);
            if (netWriteBuffer[sockfd].first.empty()) {
                return 0;
            }
            auto& buffer=netWriteBuffer[sockfd].first.front();
            int &offset=netWriteBuffer[sockfd].second;
            auto *data=buffer.data()+offset;
            int nsize=patcher->write(sockfd,data,buffer.length()-offset);
            offset+=nsize;
            if (offset==buffer.length()) {
                netWriteBuffer[sockfd].first.pop_front();
                offset=0;
            }
            return 0;
        };
        auto onRead=[&](int index,int sockfd) {
            std::lock_guard<std::mutex> lck(rwLock);
            auto n=patcher->read(sockfd,buf,MAXLINE);
            if (n<=0)
            {
                patcher->remove(index);
                return 0;
            }   
            int proxyfd=actual2proxyMap[sockfd];
            proxyWriteBuffer[proxyfd].first.push_back(std::string(buf,n));
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
        patcher->doDispatch(onRead,onWrite,onDispatch,nullptr);
    });
    proxyThread.join();
    netThread.join();
}
tcpServer::~tcpServer()
{
}
