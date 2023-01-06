#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "registry.h"
#include "json/serialization.h"
#include "config.h"
#include <functional>

class dispatcher;
class dispatcherMethodRegisry {
public:
    static dispatcherMethodRegisry& instance() {
        static dispatcherMethodRegisry instance_;
        return instance_;
    }
    void set(std::string key,std::function<void(dispatcher*,JsonParser&)> value) {
        initMap[key]=value;
    }
    void init(std::string key,dispatcher* patcher,JsonParser& json) {
        initMap[key](patcher,json);
    }
private:
    std::map<std::string,std::function<void(dispatcher*,JsonParser&)>> initMap;
};

namespace typeHelper {

template <typename F,int N,int now,typename ...Args>
constexpr auto bindWithPlaceHolder(F functor,Args... args) {
    if constexpr(now>N+1) {
        return std::bind(functor,args...);
    } else {
        std::_Placeholder<now> placeholder;
        return bindWithPlaceHolder<F,N,now+1,Args...,decltype(placeholder)>(functor,args...,placeholder);
    }
}
template <int length>
std::tuple<> genType(JsonParser&json) {
    return std::make_tuple();
}
template <int length,typename first,typename... args>
std::tuple<first,args...> genType(JsonParser& json) {
    auto now=serialize::doUnSerialize<typename std::remove_reference<first>::type>(json["arg"+std::to_string(length)]);
    return std::tuple_cat(std::make_tuple(now),genType<length+1,args...>(json));
}
template <typename returnType,typename... args>
std::tuple<args...> _genType(JsonParser& json,std::function<returnType(args...)>) {
    return genType<1,args...>(json);
}
template <typename returnType,typename... args>
std::tuple<args...> _genType(JsonParser& json,returnType(args...)) {
    return genType<1,args...>(json);
}
}

template <typename T>
class registry<dispatcher,T> {
public:
    registry() {
        static_assert(std::is_base_of<dispatcher,T>::value,"typename should be base of dispatcher");
    }
private:
    template <typename U,typename V>
    static void bindInitMethod_(U method,V functor) {
        std::string demangled=getDemangleName<T>();
        dispatcherMethodRegisry::instance().set(demangled,[method,functor](dispatcher* patcher,JsonParser &json){
            auto tuple=typeHelper::_genType(json,functor);
            std::apply([&](auto ...args){
                std::bind(method,dynamic_cast<T*>(patcher),args...)();
            },tuple);
            return;
        });
    }
    template <typename F,typename T_>
    static void bindInitMethod(F T_::* method) {
        return bindInitMethod_(method,(std::function<F>)nullptr);
    }
protected:
    class registry_ {
    public:
        __attribute((used)) registry_() {
            std::string demangled=getDemangleName<T>();
            auto ptr=std::make_shared<T>();
            bindInitMethod(&T::init);
            dynamicLoader<dispatcher>::instance().set(demangled,[](){
                return std::make_shared<T>();
            });
        }
    };
    __attribute((used)) static registry_ clazz_;
};

template <typename T>
typename registry<dispatcher,T>::registry_ registry<dispatcher,T>::clazz_;

template <typename T>
using dispatcherRegistry=registry<dispatcher,T>;

class dispatcher
{
protected:
    int maxClient;
    virtual void doInit(JsonParser& json) {
        char buffer[256]={0};
        const char *mangled=typeid(*this).name();
        size_t length=256;
        abi::__cxa_demangle(mangled,buffer,&length,nullptr);
        
        dispatcherMethodRegisry::instance().init(buffer,this,json);
    }
public:
    static std::shared_ptr<dispatcher> get(JsonParser& config) {
        auto patcher=dynamicLoader<dispatcher>::instance().get(config["name"].toString());
        patcher->doInit(config["params"]);
        return patcher;
    }
    void init(int maxClient) {
        this->maxClient=maxClient;
    }
    bool fullWrite(int sockfd,const char* buf,int size) {
        int wrote=0;
        while (wrote<size) {
            int wrote_=write(sockfd,buf+wrote,size-wrote);
            if (wrote_<0) {
                return false;
            }
            wrote+=wrote_;
        }
        return true;
    }
    virtual int connect()=0;
    virtual int insert(int fd)=0;
    virtual int remove(int index)=0;
    virtual int read(int sockfd,char* buf,int size)=0;
    virtual int write(int sockfd,const char* buf,int size)=0;
    virtual void doDispatch(std::function<int(int,int)> onRead,std::function<int(int)> onWrite,std::function<void()> onDispatch=nullptr,std::function<int(int)> onConnect=nullptr)=0;//index,sockfd
    
};
#if defined(_WIN32) || defined(_WIN64)
#include "winSelectDispatcher.h"
#else
#include "pollDispatcher.h"
#endif
#endif
