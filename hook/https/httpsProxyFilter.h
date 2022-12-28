#ifndef HTTPSPROXYFILTER_H
#define HTTPSPROXYFILTER_H
#include "../http/httpRequestFilter.h"
#include "SSLManager.h"

extern std::unordered_map<int,std::shared_ptr<SSLManager>> managerMap; 

class httpsProxyFilter : public httpRequestFilter,public filterRegistry<httpsProxyFilter> {
public:
    __attribute((used)) httpsProxyFilter() {
        
    }
    __attribute((used)) virtual std::string filter(int sockfd,std::string content,filterReason& reason);
private:
    std::unordered_map<int,int> cache;
};
#endif