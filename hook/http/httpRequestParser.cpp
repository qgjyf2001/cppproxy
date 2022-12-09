#include "httpRequestParser.h"
std::string httpRequestParser::parseToken(std::string& str,int &pos,int &length,char token)
{
    int origin=pos;
    while (pos<length&&str[pos]!=token)
        pos++;
    if (pos==length&&str[pos]!=token)
        throw std::runtime_error("invalid http request/response!");
    std::string result=str.substr(origin,pos-origin);
    pos+=1;
    return result;
}
void httpRequestParser::parse(std::string httpRequest)
{
    int pos=0;
    int length=httpRequest.length();
    method=parseToken(httpRequest,pos,length,' ');
    url=parseToken(httpRequest,pos,length,' ');
    version=parseToken(httpRequest,pos,length,'\r');
    while (++pos!=length&&httpRequest[pos]!='\r')
    {
        auto key=parseToken(httpRequest,pos,length,':');
        headers[key]=parseToken(httpRequest,pos,length,'\r');
    }
    pos+=2;
    if (pos!=length)
        text=httpRequest.substr(pos,length-pos);
    return;
}