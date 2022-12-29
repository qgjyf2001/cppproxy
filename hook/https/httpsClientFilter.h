#ifndef HTTPSCLIENTFILTER_H
#define HTTPSCLIENTFILTER_H
#include "../http/httpResponseFilter.h"
#include "SSLManager.h"

extern std::unordered_map<int,std::shared_ptr<SSLManager>> managerMap; 

class httpsClientFilter : public httpResponseFilter,public filterRegistry<httpsClientFilter> {
public:
    __attribute((used)) httpsClientFilter() {
        
    }
    __attribute((used)) virtual std::string filter(int sockfd,std::string content,filterReason& reason);
};
#endif