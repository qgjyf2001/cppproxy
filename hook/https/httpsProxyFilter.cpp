#include "httpsProxyFilter.h"
#include "tcpServer.h"
std::string httpsProxyFilter::filter(int sockfd,std::string content,bool& needFilter) {
    sockfd=tcpServer::proxy2actualMap[sockfd];
    if (managerMap.find(sockfd)!=managerMap.end()) {
        managerMap[sockfd]=std::make_shared<SSLManager>();
    }
    auto &manager=*managerMap[sockfd];
    return manager.writeProxy(content,[](std::string s){
        std::cout<<s<<std::endl;
        return s;
    });
}