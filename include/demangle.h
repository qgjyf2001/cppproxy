#ifndef DEMANGLE_H
#define DEMANGLE_H

#include <cxxabi.h>
#include <iostream>

template <typename T>
std::string getDemangleName() {
    char buffer[256]={0};
    const char *mangled=typeid(T).name();
    size_t length=256;
    abi::__cxa_demangle(mangled,buffer,&length,nullptr);
    return buffer;
}
#endif