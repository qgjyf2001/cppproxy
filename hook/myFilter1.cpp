#include "http/httpRequestFilter.h"
class myFilter1 : public httpRequestFilter,public filterRegistry<myFilter1>{
public:
    __attribute((used)) myFilter1() {

    }
protected:
    virtual void doFilter(httpRequestParser& request) { 
        auto *filter_=new commonFilter;
        auto& wrappedFilter=filter_->filterURL("/chfs/upload").overwriteHeader("Host","127.0.0.1:8081").replaceContent("gitToken.txt","uploads.txt");
        wrappedFilter(request);
        delete filter_; 
    }
};
