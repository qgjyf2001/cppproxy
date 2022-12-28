#include "httpsClientFilter.h"
std::unordered_map<int,std::shared_ptr<SSLManager>> managerMap;
std::string httpsClientFilter::filter(int sockfd,std::string content,filterReason& reason) {
    auto &manager=*managerMap[sockfd];
    try {
        return manager.writeClient(content,reason);
    } catch (std::exception& e) {
        std::cout<<"reset by client:"<<sockfd<<std::endl;
        managerMap[sockfd].reset(new SSLManager());
        throw e;
    }
}