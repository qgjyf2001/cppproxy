#ifndef HTTPREQUESTPARSER_H
#define HTTPREQUESTPARSER_H
#include <iostream>
#include <map>

#include "json/serialization.h"
SERIALIZECLASS(httpRequestParser)
{
private:
    static std::string parseToken(std::string& str,int &pos,int &length,char token);
public:
    SERIALIZEOBJECT(std::string,method);
    SERIALIZEOBJECT(std::string,url);
    SERIALIZEOBJECT(std::string,version);
    SERIALIZEOBJECT(std::string,text);
    SERIALIZEOBJECT(decltype(std::map<std::string,std::string>{}),headers);
    void parse(std::string httpRequest);
    std::string toString() {
        std::string result=method+" "+url+" "+version+"\r\n";
        for (auto &&key:headers) {
            result+=key.first+":"+key.second+"\r\n";
        }
        result+="\r\n";
        result+=text;
        return result;
    }
};
#endif