#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <thread>
#include <future>

#include "safeQueue.h"
#include <sstream>
#include "qtstreambuf.h"
#include "config.h"

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
    std::thread manageThread,proxyThread,streamThread;
    safeQueue<std::promise<int>> connections;
    bool quit=false;
    std::shared_ptr<qtStreamBuf> buffer;
    void loadConfig(std::string file);
    JsonParser dumpConfig();
signals:
    void AppendText(const QString &text);
private slots:
    void SlotAppendText(const QString &text);
    void on_loadConfig_triggered();

    void on_saveConfig_triggered();

public:
    void Append(const QString &text);
};
#endif // MAINWINDOW_H
