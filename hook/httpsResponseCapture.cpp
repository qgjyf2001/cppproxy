#include "https/httpsClientFilter.h"
class httpsResponseCapture : public httpsClientFilter,public filterRegistry<httpsResponseCapture>{
public:
    __attribute((used)) httpsResponseCapture() {

    }
protected:
    __attribute((used)) virtual void doFilter(httpResponseParser& response) { 
        std::cout<<"response<<<<<<<<<<<<<"<<std::endl;
        std::cout<<serialize::doSerialize(response)<<std::endl;
    }
};
