#include "httpsProxyFilter.h"
#include "tcpServer.h"
std::string httpsProxyFilter::filter(int sockfd,std::string content,bool& needFilter) {
    sockfd=tcpServer::actual2proxyMap[sockfd];
    if (managerMap.find(sockfd)==managerMap.end()) {
        managerMap[sockfd]=std::make_shared<SSLManager>();
    }
    auto &manager=*managerMap[sockfd];
    try {
        return manager.writeProxy(content,[](std::string s){
            std::cout<<s<<std::endl;
            return s;
        });
    } catch (...) {
        std::cout<<"reset:"<<sockfd<<std::endl;
        managerMap[sockfd].reset(new SSLManager());
        throw std::runtime_error("SSL error");
    }
}