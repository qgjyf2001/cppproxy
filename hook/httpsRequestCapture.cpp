#include "https/httpsProxyFilter.h"
class httpsRequestCapture : public httpsProxyFilter,public filterRegistry<httpsRequestCapture>{
public:
    __attribute((used)) httpsRequestCapture() {

    }
protected:
    __attribute((used)) virtual void doFilter(httpRequestParser& request) { 
        std::cout<<"request>>>>>>>>>>>>>"<<std::endl;
        std::cout<<serialize::doSerialize(request)<<std::endl;
    }
};
