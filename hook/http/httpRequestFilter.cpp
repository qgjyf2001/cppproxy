#include "httpRequestFilter.h"
std::string httpRequestFilter::filter(int sockfd,std::string content,filterReason& reason) {
    httpRequestParser parser;
    if (map.find(sockfd)!=map.end()) {
        parser=std::move(map[sockfd]);
        parser.text+=content;
        map.erase(sockfd);
        goto checkLength;
    }
    try
    {
        parser.parse(content);
    }
    catch(const std::exception& e)
    {
        std::cerr<< "http request parse error:" << e.what() << '\n';
        return content;
    }
    checkLength:
    int textLength=parser.text.length();
    auto &headers=parser.headers;
    if (headers.find("Content-Length")!=headers.end() && std::atoi(headers["Content-Length"].data()+1)!=textLength) {
        map[sockfd]=std::move(parser);
        reason=filterReason::NOTUSED;
        return "";
    } 
    doFilter(parser);
    return parser.toString();
}