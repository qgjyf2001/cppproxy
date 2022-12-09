#ifndef HTTPPARSER_H
#define HTTPPARSER_H
#include <iostream>
#include <map>
class httpRequestParser
{
private:
    static std::string parseToken(std::string& str,int &pos,int &length,char token);
public:
    std::string method,url,version,text;
    std::map<std::string,std::string> headers;
    void parse(std::string httpRequest);
    void resetContent(std::string& content) {
        text=content;
        headers["Content-Length"]=std::to_string(content.length());
    }
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