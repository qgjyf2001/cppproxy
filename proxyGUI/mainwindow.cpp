#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "tcpServer.h"
#include "clientConnectionManager.h"
#include "config.h"

#include <sstream>
#include <QMessageBox>
void MainWindow::SlotAppendText(const QString &text)
{
    static QString textbuf;
    textbuf+=text;
    ui->logText->setText(textbuf);
}
void MainWindow::Append(const QString &text)
{
    emit AppendText(text);
}
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(this,SIGNAL(AppendText(QString)),this,SLOT(SlotAppendText(QString)));
    buffer=std::make_shared<qtStreamBuf>(this);
    new (&std::cout) std::ostream(buffer.get());
#if defined(_WIN32) || defined(_WIN64)
    WSADATA wsaData = {0};
    int nRet = 0;
    if(SOCKET_ERROR == WSAStartup(MAKEWORD(2,2), &wsaData))
    {
        QMessageBox::information(nullptr,"错误","网络库初始化失败");
    }
    config::instance().init("../cppproxy/config/client_windows.json");
#endif
}

MainWindow::~MainWindow()
{
    if (remoteIP!="") {
              std::cout<<"logout..."<<std::endl;
              quit=true;
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
                  if(::connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
                  {
                      std::cerr<<"connect error at:"<<__FILE__<<",line"<<__LINE__<<std::endl;;
                  }
  #if defined(_WIN32) || defined(_WIN64)
                  closesocket(sockfd);
  #else
                  close(sockfd);
  #endif
              }
              manageThread.join();
              std::cout<<"done"<<std::endl;
    }
    delete ui;
}


void MainWindow::on_pushButton_clicked()
{
    if (remoteIP=="") {
        remoteIP=ui->remoteIP->text().toStdString();
        remotePort=ui->remotePort->text().toInt();
        remoteProxyPort=ui->remoteProxyPort->text().toInt();
        forwardIP=ui->forwardIP->text().toStdString();
        forwardPort=ui->forwardPort->text().toInt();
        proxyThread=std::thread([&](){
                tcpServer server;
                server.doProxy(connections,tcpServer::CLIENT,&forwardPort,&forwardIP);
            });
        manageThread=std::thread([&](std::string password){
                clientConnectionManager manager(remoteIP,remotePort);
                manager.doManage(password,connections,&quit);
            },ui->password->text().toStdString());
        ui->pushButton->setText("修改代理");
    } else {
        forwardIP=ui->forwardIP->text().toStdString();
        forwardPort=ui->forwardPort->text().toInt();
        std::cout<<"reset forward conection, IP:"<<forwardIP<<",port:"<<forwardPort<<std::endl;
    }
}
