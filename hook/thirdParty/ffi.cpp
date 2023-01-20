#include <filesystem>
#include <iostream>
#include <dlfcn.h>
#include "../filter.h"

struct Ret {
	unsigned char* ptr;
	unsigned int len;
	unsigned int cap;
};

class ffiFilter : public baseFilter{
public:
    ffiFilter(std::function<std::string(int,std::string,filterReason&)> functor):functor(functor) {

    }
    virtual std::string filter(int sockfd,std::string content,filterReason &reason) {
        return functor(sockfd,content,reason);
    }
private:
    std::function<std::string(int,std::string,filterReason&)> functor;
};

class ffi {
protected:
    class invoke {
    public:
        invoke() {
            std::filesystem::directory_iterator thirdPartyPath(std::filesystem::current_path()/"thirdParty");
            for (auto &file:thirdPartyPath) {
                Ret (*filterFunc) (int,unsigned char* ,size_t,int*);
                void (*freeFunc) (Ret);
                auto thirdParty=dlopen(file.path().c_str(),RTLD_LAZY);
	            filterFunc=(decltype(filterFunc))dlsym(thirdParty,"filter");
	            freeFunc=(decltype(freeFunc))dlsym(thirdParty,"rust_module_free");
                std::string moduleName=file.path().filename();
                std::cout<<"regist module "<<moduleName<<std::endl;
                dynamicLoader<baseFilter>::instance().set(moduleName,[filterFunc,freeFunc](){    
                    auto functor=[filterFunc,freeFunc](int sockfd,std::string content,filterReason& reason) {
                        int _reason;
                        auto ret=filterFunc(sockfd,(unsigned char*)content.data(),content.length(),&_reason);
                        reason=(filterReason)_reason;
                        std::string result((char*)ret.ptr,ret.len);
                        freeFunc(ret);
                        return result;
                    };
                    return std::make_shared<ffiFilter>(functor);
                });
            }
        }
    };
    __attribute((used)) static invoke clazz_;
};
ffi::invoke ffi::clazz_;