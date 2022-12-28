#include "httpsProxyFilter.h"
#include "tcpServer.h"
std::string httpsProxyFilter::filter(int sockfd,std::string content,filterReason& reason) {
    int sockfd_=tcpServer::actual2proxyMap[sockfd];
    if (cache.find(sockfd)==cache.end()||cache[sockfd]!=sockfd_) {
        cache[sockfd]=sockfd_;
        managerMap[sockfd_]=std::make_shared<SSLManager>();
    }        
    sockfd=sockfd_;
    auto &manager=*managerMap[sockfd];
    try {
        return manager.writeProxy(content,reason,[this,sockfd,&reason](std::string s){
            return httpRequestFilter::filter(sockfd,s,reason);
        });
    } catch (std::exception& e) {
        std::cout<<"reset:"<<sockfd<<std::endl;
        managerMap[sockfd].reset(new SSLManager());
        throw e;
    }
}