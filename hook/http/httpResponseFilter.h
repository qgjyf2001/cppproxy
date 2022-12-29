#ifndef HTTPRESPONSEFILTER_H
#define HTTPRESPONSEFILTER_H
#include "../filter.h"
#include "httpResponseParser.h"
#include "utils/stringUtils.h"
#include <vector>
class httpResponseFilter : public baseFilter,public filterRegistry<httpResponseFilter> {
public:
    __attribute((used)) httpResponseFilter() {
    }
    __attribute((used)) virtual void reset(int sockfd) {
        map.erase(sockfd);
    }
    __attribute((used)) virtual std::string filter(int sockfd,std::string content,filterReason& reason);
protected:
    virtual void doFilter(httpResponseParser& response) {

    }
    class commonFilter : std::enable_shared_from_this<commonFilter> {    
        protected:
            bool notFilter;
        public:
            void operator()(httpResponseParser& response) {
                notFilter=false;
                for (auto filter_:filters) {
                    if (notFilter) {
                        continue;
                    }
                    filter_(response);
                }
                if (response.headers.find("Content-Length")!=response.headers.end()) {
                    response.headers["Content-Length"]=" "+std::to_string(response.text.length());
                }
            }
            commonFilter& replaceContent(std::string originValue,std::string targetValue) {
                filters.push_back([&](httpResponseParser& response){
                    replaceString(response.text,originValue,targetValue);
                });
                return *this;
            }
            commonFilter& replaceHeader(std::string header,std::string originValue,std::string targetValue) {
                filters.push_back([&](httpResponseParser& response){
                    replaceString(response.headers[header],originValue,targetValue);
                });
                return *this;
            }
            commonFilter& overwriteHeader(std::string header,std::string value){
                filters.push_back([&](httpResponseParser& response){
                    response.headers[header]=" "+value;
                });
                return *this;
            }
            commonFilter& overwriteContent(std::string content) {
                filters.push_back([&](httpResponseParser& response){
                    response.text=content;
                });
                return *this;
            }
            // commonFilter& filterURL(std::string url) {
            //     filters.push_back([&](httpResponseParser& response){
            //         this->notFilter=(response.url!=url);
            //     });
            //     return *this;
            // }
            private:
                std::vector<std::function<void(httpResponseParser&)>> filters;
        };
private:
    std::unordered_map<int,httpResponseParser> map;
};
#endif