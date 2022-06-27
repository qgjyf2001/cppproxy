#include "tcpServer.h"
#include "clientConnectionManager.h"
std::string remoteIP="127.0.0.1";
int remotePort=8081;
int remoteProxyPort=8000;
int main() {
    safeQueue<int> connections;
    bool quit=false;
    std::thread manageThread([&](){
        clientConnectionManager manager(remoteIP,remotePort);
        manager.doManage("123456",connections,&quit);
    });
    std::thread proxyThread([&](){
        tcpServer server;
        server.doProxy(connections,tcpServer::CLIENT,22,"127.0.0.1");
    });
    while (true) {
        char buf[256];
        std::cin.getline(buf,256);
        std::string command(buf);
        std::string _quit="quit";
        if (command.substr(0,_quit.length())==_quit) {
            std::cout<<"logout..."<<std::endl;
            quit=true;
            for (int i=0;i<2;i++) {
                int sockfd = socket(AF_INET, SOCK_STREAM, 0);//向真实端口发起连接
                sockaddr_in servaddr;
                bzero(&servaddr, sizeof(servaddr));
                servaddr.sin_family = AF_INET;
                servaddr.sin_port = htons(remoteProxyPort);
                inet_pton(AF_INET, remoteIP.c_str(), &servaddr.sin_addr);
                if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
                {
                    std::cerr<<"connect error at:"<<__FILE__<<",line"<<__LINE__<<std::endl;;
                }
                close(sockfd);
            }
            manageThread.join();
            std::cout<<"done"<<std::endl;
            break;
        }
    }
}