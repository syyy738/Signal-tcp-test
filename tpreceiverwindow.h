#ifndef TPRECEIVERWINDOW_H
#define TPRECEIVERWINDOW_H

#include <QWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include <QVector>
#include <QThread>
#include <QMutex>

namespace Ui {
class TcpReceiverWindow;
}

class DataProcessor : public QObject
{
    Q_OBJECT

public slots:
    void processData(QTcpSocket *socket, const QByteArray &data);

signals:
    void dataProcessed(const QString &result);
};

class TcpReceiverWindow : public QWidget
{
    Q_OBJECT

public:
    explicit TcpReceiverWindow(QWidget *parent = nullptr);
    ~TcpReceiverWindow();

private slots:
    void on_startButton_clicked();
    void on_stopButton_clicked();
    void onNewConnection();
    void onClientDisconnected();
    void onReadyRead();
    void onDataProcessed(const QString &result);

private:
    Ui::TcpReceiverWindow *ui;
    QTcpServer *tcpServer;
    QList<QTcpSocket*> clientSockets;
    QThread *workerThread;
    DataProcessor *dataProcessor;
    QMutex socketMutex;
};

#endif // TPRECEIVERWINDOW_H
