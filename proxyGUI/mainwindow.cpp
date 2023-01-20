#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "tcpServer.h"
#include "clientConnectionManager.h"

#include <sstream>
#include <QMessageBox>
#include <QFileDialog>
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
void MainWindow::loadConfig(std::string file) {
    std::ifstream fin;
    fin.open(file, std::ios::in);
    std::stringstream buf;
    buf << fin.rdbuf();
    std::string content=buf.str();
    JsonParser json(&content);
    ui->password->setText(QString::fromStdString(json["password"].toString()));
    ui->remoteIP->setText(QString::fromStdString(json["remoteIP"].toString()));
    ui->remotePort->setText(QString::number(json["remotePort"].toInt()));
    ui->remoteProxyPort->setText(QString::number(json["remoteProxyPort"].toInt()));
    ui->forwardIP->setText(QString::fromStdString(json["forwardIP"].toString()));
    ui->forwardPort->setText(QString::number(json["forwardPort"].toInt()));
    ui->proxyConfig->setPlainText(QString::fromStdString(json["proxyDispatcher"]["params"]));
    ui->clientConfig->setPlainText(QString::fromStdString(json["clientDispatcher"]["params"]));
    ui->proxyClass->setCurrentText(QString::fromStdString(json["proxyDispatcher"]["name"]));
    ui->clientClass->setCurrentText(QString::fromStdString(json["clientDispatcher"]["name"]));
}
JsonParser MainWindow::dumpConfig() {
    JsonParser json;
    json["password"]=ui->password->text().toStdString();
    json["remoteIP"]=ui->remoteIP->text().toStdString();
    json["remotePort"]=ui->remotePort->text().toInt();
    json["remoteProxyPort"]=ui->remoteProxyPort->text().toInt();
    json["forwardIP"]=ui->forwardIP->text().toStdString();
    json["forwardPort"]=ui->forwardPort->text().toInt();
    json["proxyDispatcher"]["name"]=ui->proxyClass->currentText().toStdString();
    json["clientDispatcher"]["name"]=ui->clientClass->currentText().toStdString();
    std::string proxyConfig=ui->proxyConfig->toPlainText().toStdString();
    json["proxyDispatcher"]["params"]=JsonParser(&proxyConfig);
    std::string clientConfig=ui->clientConfig->toPlainText().toStdString();
    json["clientDispatcher"]["params"]=JsonParser(&clientConfig);
    return json;

}
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(this,SIGNAL(AppendText(QString)),this,SLOT(SlotAppendText(QString)));
    buffer=std::make_shared<qtStreamBuf>(this);
    new (&std::cout) std::ostream(buffer.get());

    QStringList classList;
    for (auto [key,_]:dynamicLoader<dispatcher>::instance().registryMap) {
        classList.append(QString::fromStdString(key));
    }
    ui->proxyClass->addItems(classList);
    ui->clientClass->addItems(classList);
#if defined(_WIN32) || defined(_WIN64)
    WSADATA wsaData = {0};
    int nRet = 0;
    if(SOCKET_ERROR == WSAStartup(MAKEWORD(2,2), &wsaData))
    {
        QMessageBox::information(nullptr,"错误","网络库初始化失败");
    }
    loadConfig("../cppproxy/config/client_windows.json");
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
    {
        config::instance().json=std::move(dumpConfig());
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
       // ui->pushButton->setText("修改代理");
    }
}

void MainWindow::on_loadConfig_triggered()
{
    QString fileName=QFileDialog::getOpenFileName(this,"加载配置",".","Json File(*.json)");
    loadConfig(fileName.toStdString());
}

void MainWindow::on_saveConfig_triggered()
{
    QString fileName = QFileDialog::getSaveFileName(this,"保存配置",".","Json File(*.json)");
    std::ofstream fout(fileName.toStdString());
    fout<<dumpConfig();
    fout.close();
}
