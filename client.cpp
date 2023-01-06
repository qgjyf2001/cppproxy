#ifdef TCPSERVER
#include "tcpServer.h"
#else
#include "udpServer.h"
#endif
#include "clientConnectionManager.h"
#include "config.h"
int main() {
#if defined(_WIN32) || defined(_WIN64)
    config::instance().init("./config/client_windows.json");
#else
    config::instance().init("./config/client_linux.json");
#endif
    std::string remoteIP=config::instance().json["remoteIP"].toString();
    int remotePort=config::instance().json["remotePort"].toInt();
    int remoteProxyPort=config::instance().json["remoteProxyPort"].toInt();
#if defined(_WIN32) || defined(_WIN64)
    WSADATA wsaData = {0};
    int nRet = 0;
    if(SOCKET_ERROR == WSAStartup(MAKEWORD(2,2), &wsaData))
    {
        std::cerr << "fail to WSAStartUp"<<std::endl;
        return -1;
    }
#endif
    safeQueue<std::promise<int>> connections;
    bool quit=false;
    std::thread manageThread([&](){
        clientConnectionManager manager(remoteIP,remotePort);
        manager.doManage(config::instance().json["password"].toString(),connections,&quit);
    });
    std::string forwardIP=config::instance().json["forwardIP"].toString();;
    int forwardPort=config::instance().json["forwardPort"].toInt();;
    std::thread proxyThread([&](){
#ifdef TCPSERVER
        tcpServer server;
        server.doProxy(connections,tcpServer::CLIENT,&forwardPort,&forwardIP);
#else
        udpServer server;
        server.doProxy(connections,udpServer::CLIENT,&forwardPort,&forwardIP);
#endif
    });
    while (true) {
        char buf[256];
        std::cin.getline(buf,256);
        std::string command(buf);
        std::string _quit="quit";
        if (command.substr(0,_quit.length())==_quit) {
            std::cout<<"logout..."<<std::endl;
            quit=true;
#ifdef TCPSERVER
            {
                int sockfd = socket(AF_INET, SOCK_STREAM, 0);//向真实端口发起连接
                sockaddr_in servaddr;
                memset(&servaddr,0, sizeof(servaddr));
                servaddr.sin_family = AF_INET;
                servaddr.sin_port = htons(remoteProxyPort);
                
#if defined(_WIN32) || defined(_WIN64)
                servaddr.sin_addr.S_un.S_addr = inet_addr(remoteIP.c_str());	
#else
                inet_pton(AF_INET, remoteIP.c_str(), &servaddr.sin_addr);
#endif
                if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
                {
                    std::cerr<<"connect error at:"<<__FILE__<<",line"<<__LINE__<<std::endl;;
                } 
#if defined(_WIN32) || defined(_WIN64)
                closesocket(sockfd);
#else
                close(sockfd);
#endif
            }
#else
            int sockfd=socket(PF_INET, SOCK_DGRAM, 0);//向真实端口发起连接
            sockaddr_in servaddr;
            memset(&servaddr,0, sizeof(servaddr));
            servaddr.sin_family = AF_INET;
            servaddr.sin_port = htons(remoteProxyPort);
            inet_pton(AF_INET, remoteIP.c_str(), &servaddr.sin_addr);
            if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
            {
                std::cerr<<"connect error"<<std::endl;
            }
            write(sockfd,"\n",1);
            close(sockfd);
#endif
            manageThread.join();
            std::cout<<"done"<<std::endl;
            break;
        }
    }
}
