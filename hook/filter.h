#ifndef FILTER_H
#define FILTER_H
#include "../dispatcher/dispatcher.h"
#include <cxxabi.h>
#include <unordered_map>
#include <memory>

class baseFilter;
inline std::unordered_map<std::string,std::shared_ptr<baseFilter>> filterMap;
template <typename T>
class filterRegistry {
public:
    filterRegistry() {
        static_assert(std::is_base_of<baseFilter,T>::value,"typename should be base of baseFilter");
    }
protected:
    class registry {
    public:
        registry() {
            char buffer[256]={0};
            const char *mangled=typeid(T).name();
            size_t length=256;
            abi::__cxa_demangle(mangled,buffer,&length,nullptr);
            std::string demangled=buffer;
            filterMap[demangled]=std::make_shared<T>();
        }
    };
    __attribute((used)) static registry clazz_;
};
template <typename T>
typename filterRegistry<T>::registry filterRegistry<T>::clazz_;
class baseFilter {
public:
    virtual void reset(int sockfd) {

    }
    virtual std::string filter(int sockfd,std::string content,bool &needFilter) {
        return std::move(content);
    }
};
template <typename T>
class filter : public T {
public:
    template <typename... Args>
    filter(std::vector<std::string> filters,Args... args):T(std::forward<Args>(args)...) {
        for (auto filter_:filters) {
            if (filterMap.find(filter_)==filterMap.end()) {
                std::cerr<<"filter "<<filter_<<" not registered!"<<std::endl;
                continue;
            }
            this->filters.push_back(filterMap[filter_]);
        }
    }
    virtual int write(int sockfd,const char* buf,int size) {
        std::string content(buf,size);
        int offset=0;
        bool needFilter=true;
        if (cache.find(sockfd)!=cache.end()) {
            content=std::move(cache[sockfd].first);
            offset=cache[sockfd].second;
            cache.erase(sockfd);
        }
        for (auto filter_:filters) {
            content=filter_->filter(sockfd,content,needFilter);
        }
        if (!needFilter) {
            return size;
        }
        int len=content.length()-offset;
        int n=T::write(sockfd,content.data()+offset,std::min(MAXLINE,len));
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
#endif