#ifndef HTTPRESPONSEPARSER_H
#define HTTPRESPONSEPARSER_H
#include <iostream>
#include <map>

#include "json/serialization.h"
SERIALIZECLASS(httpResponseParser)
{
private:
    static std::string parseToken(std::string& str,int &pos,int &length,char token);
public:
    SERIALIZEOBJECT(std::string,version);
    SERIALIZEOBJECT(std::string,code);
    SERIALIZEOBJECT(std::string,information);
    SERIALIZEOBJECT(std::string,text);
    SERIALIZEOBJECT(decltype(std::map<std::string,std::string>{}),headers);
    void parse(std::string httpResponse);
    std::string toString() {
        std::string result=version+" "+code+" "+information+"\r\n";
        for (auto &&key:headers) {
            result+=key.first+":"+key.second+"\r\n";
        }
        result+="\r\n";
        result+=text;
        return result;
    }
};
#endif