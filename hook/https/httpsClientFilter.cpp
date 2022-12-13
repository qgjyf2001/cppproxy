#include "httpsClientFilter.h"
std::unordered_map<int,std::shared_ptr<SSLManager>> managerMap;
std::string httpsClientFilter::filter(int sockfd,std::string content,bool& needFilter) {
    if (managerMap.find(sockfd)==managerMap.end()) {
        managerMap[sockfd]=std::make_shared<SSLManager>();
    }
    auto &manager=*managerMap[sockfd];
    try {
        return manager.writeClient(content);
    } catch (...) {
        std::cout<<"reset by client:"<<sockfd<<std::endl;
        managerMap[sockfd].reset(new SSLManager());
        throw std::runtime_error("SSL error");
    }
}