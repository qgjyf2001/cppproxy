#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <thread>

#include "safeQueue.h"
#include <sstream>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_clicked();

private:
    std::string remoteIP;
    int remotePort;
    int remoteProxyPort=8000;
    Ui::MainWindow *ui;
    std::string forwardIP;
    int forwardPort;
    std::thread manageThread,proxyThread;
    safeQueue<int> connections;
    bool quit=false;
};
#endif // MAINWINDOW_H
