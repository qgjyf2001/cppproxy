#include "httpResponseParser.h"
std::string httpResponseParser::parseToken(std::string& str,int &pos,int &length,char token)
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
void httpResponseParser::parse(std::string httpResponse)
{
    int pos=0;
    int length=httpResponse.length();
    version=parseToken(httpResponse,pos,length,' ');
    code=parseToken(httpResponse,pos,length,' ');
    information=parseToken(httpResponse,pos,length,'\r');
    while (++pos!=length&&httpResponse[pos]!='\r')
    {
        auto key=parseToken(httpResponse,pos,length,':');
        headers[key]=parseToken(httpResponse,pos,length,'\r');
    }
    pos+=2;
    if (pos!=length)
        text=httpResponse.substr(pos,length-pos);
    return;
}