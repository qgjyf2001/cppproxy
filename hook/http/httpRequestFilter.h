#ifndef HTTPREQUESTFILTER_H
#define HTTPREQUESTFILTER_H
#include "../filter.h"
#include "httpRequestParser.h"
#include <vector>
static void replaceString(std::string& str, const std::string& be_replaced_str, const std::string& new_replace_str)
{
    for (std::string::size_type pos = 0; pos != std::string::npos; pos += new_replace_str.length())
    {
        pos = str.find(be_replaced_str, pos);
        if (pos != std::string::npos)
        {
            str.replace(pos, be_replaced_str.length(), new_replace_str);
        }
        else
        {
            break;
        }
    }
}
class httpRequestFilter : public baseFilter,public filterRegistry<httpRequestFilter> {
public:
    __attribute((used)) httpRequestFilter() {
    }
    __attribute((used)) virtual void reset(int sockfd) {
        map.erase(sockfd);
    }
    __attribute((used)) virtual std::string filter(int sockfd,std::string content,bool& needFilter);
protected:
    virtual void doFilter(httpRequestParser& request) {

    }
    class commonFilter : std::enable_shared_from_this<commonFilter> {    
        protected:
            bool notFilter;
        public:
            void operator()(httpRequestParser& request) {
                notFilter=false;
                for (auto filter_:filters) {
                    if (notFilter) {
                        continue;
                    }
                    filter_(request);
                }
                if (request.headers.find("Content-Length")!=request.headers.end()) {
                    request.headers["Content-Length"]=" "+std::to_string(request.text.length());
                }
            }
            commonFilter& replaceContent(std::string originValue,std::string targetValue) {
                filters.push_back([&](httpRequestParser& request){
                    replaceString(request.text,originValue,targetValue);
                });
                return *this;
            }
            commonFilter& replaceHeader(std::string header,std::string originValue,std::string targetValue) {
                filters.push_back([&](httpRequestParser& request){
                    replaceString(request.headers[header],originValue,targetValue);
                });
                return *this;
            }
            commonFilter& overwriteHeader(std::string header,std::string value){
                filters.push_back([&](httpRequestParser& request){
                    request.headers[header]=" "+value;
                });
                return *this;
            }
            commonFilter& overwriteContent(std::string content) {
                filters.push_back([&](httpRequestParser& request){
                    request.text=content;
                });
                return *this;
            }
            commonFilter& filterURL(std::string url) {
                filters.push_back([&](httpRequestParser& request){
                    this->notFilter=(request.url!=url);
                });
                return *this;
            }
            private:
                std::vector<std::function<void(httpRequestParser&)>> filters;
        };
private:
    std::unordered_map<int,httpRequestParser> map;
};
#endif