#ifndef FILTER_H
#define FILTER_H
#include "../dispatcher/dispatcher.h"
#include "registry.h"
#include <unordered_map>
#include <memory>

enum filterReason {
    NONE,
    NOTUSED,
    REUSED
};
class baseFilter {
public:
    virtual void reset(int sockfd) {

    }
    virtual std::string filter(int sockfd,std::string content,filterReason &reason) {
        return std::move(content);
    }
};
template <typename T>
using filterRegistry=registry<baseFilter,T>;
template <typename T,typename ...Args>
class filter : public T {
public:
    void init(std::vector<std::string> filters,Args... args) {
        T::init(std::forward<Args>(args)...);
        for (auto filter_:filters) {
            auto filter_ptr=dynamicLoader<baseFilter>::instance().get(filter_);
            if (filter_ptr==nullptr) {
                std::cerr<<"filter "<<filter_<<" not registered!"<<std::endl;
                continue;
            }
            this->filters.push_back(filter_ptr);
        }
    }
    virtual int write(int sockfd,const char* buf,int size) {
        std::string content(buf,size);
        int offset=0;
        filterReason reason=filterReason::NONE;
        if (cache.find(sockfd)!=cache.end()) {
            content=std::move(cache[sockfd].first);
            offset=cache[sockfd].second;
            cache.erase(sockfd);
        } else {
            for (auto filter_:filters) {
                try {
                    content=filter_->filter(sockfd,content,reason);
                    if (reason!=filterReason::NONE) {
                    break;
                    }
                } catch (std::exception& e) {
                    cache.erase(sockfd);
                    return size;
                }
            }
        }
        int len=content.length()-offset;
        int n=T::write(sockfd,content.data()+offset,std::min(MAXLINE,len));
        if (reason==filterReason::NOTUSED) {
            return size;
        }
        if (reason==filterReason::REUSED) {
            return 0;
        }
        if (n==-1) {
            for (auto filter_:filters) {
                filter_->reset(sockfd);
            }
            cache.erase(sockfd);
            return -1;
        }
        offset+=n;
        if (offset!=content.length()) {
            cache[sockfd].first=std::move(content);
            cache[sockfd].second=offset;
            return 0;
        }
        return size;
    }
private:
    std::vector<std::shared_ptr<baseFilter>> filters;
    std::unordered_map<int,std::pair<std::string,int>> cache;
};

namespace typeHelper {
    template <typename U,typename V>
    struct bindFilterWithArgs_ {

    };
    template <typename T,typename... Args>
    struct bindFilterWithArgs_<T,std::function<void(Args...)>> {
        using type=filter<T,Args...>;
    };
    template <typename T>
    struct bindFilterWithArgs {
        using type=typename bindFilterWithArgs<decltype(&T::init)>::type;
    };
    template <typename T,typename F>
    struct bindFilterWithArgs<F T::*> {
        using type=typename bindFilterWithArgs_<T,std::function<F>>::type;
    };
};
template <typename T>
class filter<T> : public typeHelper::bindFilterWithArgs<T>::type,public dispatcherRegistry<filter<T>>
{
};

template class filter<pollDispatcher>;
#endif
