#include "winSelectDispatcher.h"
int winSelectDispatcher::insert(int sockfd) {
    FD_SET(sockfd, &clientfd);
    return 0;
}
int winSelectDispatcher::remove(int i) {
    SOCKET socket_temp = clientfd.fd_array[i];
	FD_CLR(clientfd.fd_array[i],&clientfd);
	closesocket(socket_temp);
}
void winSelectDispatcher::doDispatch(std::function<int(int,int)> onRead,std::function<void()> onDispatch,std::function<int(int)> onConnect) {
    while (true)
    {
        struct timeval timeInterval;//给参数5赋值等待时间
		timeInterval.tv_sec = 0;
		timeInterval.tv_usec = 10000;
        int nready = select(0,&clientfd,NULL,NULL, &timeInterval);
        char buf[MAXLINE];
        for (int i = 0; i < clientfd.fd_count; i++) {
            onRead(i,clientfd.fd_array[i]);
        }
        if (onDispatch!=nullptr) {
            onDispatch();
        }
    }
}