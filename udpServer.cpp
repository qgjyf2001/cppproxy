#include "udpServer.h"
#include "md5Helper.h"
#include <cassert>
#include "dispatcher/dispatcher.h"
udpServer::udpServer(int proxyPort,std::string proxyIP,int maxClient)
{
#if !(defined(_WIN32) || defined(_WIN64))
    signal(SIGPIPE , SIG_IGN);
#endif
    this->proxyPort=proxyPort;
    this->proxyIP=proxyIP;
    this->maxClient=maxClient/2;
}
void udpServer::doProxy(safeQueue<std::promise<int>>& connections,serviceType type,int* port,std::string* ipAddress)
{
    auto bindNewAddr=[port_=proxyPort](){    
        int sockfd;
        sockfd = socket(PF_INET, SOCK_DGRAM, 0);
        int opt = 1;
        setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
        struct sockaddr_in my_addr;
        bzero(&my_addr, sizeof(my_addr));
        my_addr.sin_family = PF_INET;
        my_addr.sin_port = htons(port_);
        my_addr.sin_addr.s_addr = INADDR_ANY;
        if (bind(sockfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr)) == -1) {
            throw std::runtime_error("bind error");
        }
        return sockfd;
    };
    std::map<int,std::pair<std::deque<std::string>,int>> proxyWriteBuffer,netWriteBuffer;
    //init
        std::thread proxyThread([&](){
        dispatcher* patcher = new pollDispatcher(maxClient);
        int listenfd=bindNewAddr();
        patcher->insert(listenfd);
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
            //if (offset==buffer.length()) {
                proxyWriteBuffer[sockfd].first.pop_front();
                offset=0;
            //}
            return 0;
        };
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
        auto onRead=[&](int index,int sockfd){
            int n;
            if (sockfd==listenfd) {
                struct sockaddr_in peer_addr;
                socklen_t cli_len = sizeof(peer_addr);
                n=recvfrom(listenfd, buf, MAXLINE, 0, (struct sockaddr *)&peer_addr, &cli_len);
                int newfd=bindNewAddr();
                peer_addr.sin_family = PF_INET;
                connect(newfd,(struct sockaddr *) &peer_addr, sizeof(struct sockaddr));
                patcher->insert(newfd);
                if (type==SERVER)
                {
                    onConnect(newfd);
                }
                sockfd=newfd;
            } else {
                n=patcher->read(sockfd,buf,MAXLINE);
                if (n<=0)
                {
                    patcher->remove(index);
                    return 0;
                }  
            } 
            std::lock_guard<std::mutex> lck(rwLock);
            int actualfd=proxy2actualMap[sockfd];
            netWriteBuffer[actualfd].first.push_back(std::to_string(n)+" "+std::string(buf,n));
            return 0;
        };
        auto onDispatch=[&](){
                if (!connections.empty()) {
                    sockaddr_in servaddr;
                    std::cout<<"new connections:"<<(type==SERVER?"server":"client")<<std::endl;
                    std::promise<int> sockfd_;
                    connections.pop(sockfd_);
                    int sockfd=sockfd_.get_future().get();
                    
                    int connfd=socket(PF_INET, SOCK_DGRAM, 0);//向真实端口发起连接
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
                        proxyWriteBuffer.clear();
                        netWriteBuffer.clear();
                        return;
                    }
                    proxy2actualMap[connfd]=sockfd;
                    actual2proxyMap[sockfd]=connfd;

                    actualfds.push(sockfd);
                    patcher->insert(connfd);
                }
                return;
            };
        if (type==SERVER) {
            patcher->doDispatch(onRead,onWrite,nullptr,nullptr);
        } else {
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
        std::map<int,std::pair<std::string,int>> cachedBytes; 
        auto onRead=[&](int index,int sockfd) {
            std::lock_guard<std::mutex> lck(rwLock);            
            auto n=patcher->read(sockfd,buf,MAXLINE);
            if (n<=0)
            {
                patcher->remove(index);
                return 0;
            }
            std::string result;
            if (cachedBytes[sockfd].second!=0) {
                cachedBytes[sockfd].first+=std::string(buf,n);
                cachedBytes[sockfd].second-=n;
                if (cachedBytes[sockfd].second!=0) {
                    return 0;
                } else {
                    result=std::move(cachedBytes[sockfd].first);
                    cachedBytes.erase(sockfd);
                }
            } else {
                read_again:
                int pos=-1;
                for (int i=0;i<n;i++) {
                    if (buf[i]==' ') {
                        pos=i-1;
                        break;
                    }
                }
                assert(pos!=-1);
                int length=std::atoi(std::string(buf,pos+1).c_str());
                result=std::string(buf+pos+2,n-pos-2);
                if (n-pos-2 < length) {
                    cachedBytes[sockfd]=std::make_pair(result,length-(n-pos-2));
                    return 0;
                } else if (n-pos-2 > length) {
                    int proxyfd=actual2proxyMap[sockfd];
                    proxyWriteBuffer[proxyfd].first.push_back(result.substr(0,length));
                    n=result.length()-length;
                    memcpy(buf,result.data()+length,n);
                    goto read_again; // 粘包
                }
            }
            int proxyfd=actual2proxyMap[sockfd];
            proxyWriteBuffer[proxyfd].first.push_back(result);
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
udpServer::~udpServer()
{
}
