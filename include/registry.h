#ifndef REGISTRY_H
#define REGISTRY_H
#include <unordered_map>
#include <iostream>
#include <memory>
#include <functional>
#include "demangle.h"

template <typename T>
class dynamicLoader {
public:
    static dynamicLoader<T>& instance() {
        static dynamicLoader<T> instance_;
        return instance_;
    }
    void set(std::string key,std::function<std::shared_ptr<T>(void)> value) {
        registryMap[key]=value;
    }
    std::shared_ptr<T> get(std::string key) {
        if (registryMap.find(key)==registryMap.end()) {
            std::cerr<<"key "<<key<<" not registered!"<<std::endl;
            exit(-1);
        }
        return registryMap[key]();
    }
public:
    std::unordered_map<std::string,std::function<std::shared_ptr<T>(void)>> registryMap;
};

template <typename U,typename V>
class registry {
public:
    registry() {
        static_assert(std::is_base_of<U,V>::value,"typename should be base of baseFilter");
    }
protected:
    class registry_ {
    public:
        __attribute((used)) registry_() {
            std::string demangled=getDemangleName<V>();
            dynamicLoader<U>::instance().set(demangled,[](){
                return std::make_shared<V>();
            });
        }
    };
    __attribute((used)) static registry_ clazz_;
};
template <typename U,typename V>
typename registry<U,V>::registry_ registry<U,V>::clazz_;

#endif
