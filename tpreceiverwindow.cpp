#include "tpreceiverwindow.h"
#include "ui_tpreceiverwindow.h"
#include <QDebug>
#include <QHostAddress>
#include <QDataStream>
#include <QTimer>

// DataProcessor类实现
void DataProcessor::processData(QTcpSocket *socket, const QByteArray &data)
{
    QHostAddress sender = socket->peerAddress();
    quint16 senderPort = socket->peerPort();

    try {
        // 首先发送ACK回复，确保及时响应
        QByteArray reply = "ACK";
        qint64 bytesSent = socket->write(reply);
        if (bytesSent != -1) {
            if (socket->waitForBytesWritten(5000)) {
                qDebug() << QString("Sent ACK to %1:%2").arg(sender.toString()).arg(senderPort);
            } else {
                qDebug() << QString("Failed to send ACK: %1").arg(socket->errorString());
            }
        } else {
            qDebug() << QString("Failed to send ACK: %1").arg(socket->errorString());
        }

        // 解析信号数据
        QDataStream stream(data);
        // 设置字节序为大端
        stream.setByteOrder(QDataStream::BigEndian);
        
        // 读取信号类型
        quint32 signalTypeLength;
        stream >> signalTypeLength;
        QByteArray signalTypeBytes;
        signalTypeBytes.resize(signalTypeLength);
        stream.readRawData(signalTypeBytes.data(), signalTypeLength);
        QString signalType = QString::fromUtf8(signalTypeBytes);
        
        double signalFrequency, sampleFrequency, lowCutFrequency, highCutFrequency;
        quint32 dataSize;

        stream >> signalFrequency;
        stream >> sampleFrequency;
        stream >> lowCutFrequency;
        stream >> highCutFrequency;
        stream >> dataSize;

        // 读取信号数据
        QVector<double> signalData;
        for (quint32 i = 0; i < dataSize; ++i) {
            double sample;
            stream >> sample;
            signalData.append(sample);
        }

        // 读取并验证校验和
        quint32 checksum = 0;
        if (stream.atEnd()) {
            qDebug() << "Warning: No checksum found in data";
        } else {
            stream >> checksum;
        }
        
        quint32 calculatedChecksum = 0;
        for (size_t i = 0; i < data.size() - sizeof(checksum); ++i) {
            calculatedChecksum += static_cast<quint8>(data[i]);
        }

        bool checksumValid = (checksum == calculatedChecksum);

        // 构建显示结果
        QString result;
        result += "===================================\n";
        result += QString("【接收信息】 来自 %1:%2\n").arg(sender.toString()).arg(senderPort);
        result += QString("数据包大小: %1 字节\n").arg(data.size());
        result += "-----------------------------------\n";
        result += QString("信号类型: %1\n").arg(signalType);
        result += QString("信号频率: %1 Hz\n").arg(signalFrequency);
        result += QString("采样频率: %1 Hz\n").arg(sampleFrequency);
        result += QString("低频截止: %1 Hz\n").arg(lowCutFrequency);
        result += QString("高频截止: %1 Hz\n").arg(highCutFrequency);
        result += QString("数据长度: %1 样本\n").arg(dataSize);
        result += QString("校验和: %1 (").arg(checksum) + (checksumValid ? "有效" : "无效") + ")\n";
        
        // 数字信号分析
        if (!signalData.isEmpty()) {
            // 计算基本统计信息
            double minValue = signalData[0];
            double maxValue = signalData[0];
            double sum = 0;
            double sumSquared = 0;
            
            for (double sample : signalData) {
                minValue = qMin(minValue, sample);
                maxValue = qMax(maxValue, sample);
                sum += sample;
                sumSquared += sample * sample;
            }
            
            double average = sum / signalData.size();
            double variance = (sumSquared / signalData.size()) - (average * average);
            double stdDev = qSqrt(variance);
            
            // 显示统计信息
            result += "-----------------------------------\n";
            result += "【信号分析】\n";
            result += QString("最小值: %1\n").arg(minValue, 0, 'f', 6);
            result += QString("最大值: %1\n").arg(maxValue, 0, 'f', 6);
            result += QString("平均值: %1\n").arg(average, 0, 'f', 6);
            result += QString("标准差: %1\n").arg(stdDev, 0, 'f', 6);
            result += QString("信号范围: %1\n").arg(maxValue - minValue, 0, 'f', 6);
        }
        
        // 显示部分信号数据（更整齐的格式）
        result += "-----------------------------------\n";
        result += "【信号数据】\n";
        int displayCount = qMin(15, static_cast<int>(signalData.size()));
        QString dataStr;
        for (int i = 0; i < displayCount; ++i) {
            dataStr += QString("%3: %1, ").arg(signalData[i], 0, 'f', 4).arg(i);
            if ((i + 1) % 3 == 0) {
                dataStr += "\n";
            }
        }
        if (signalData.size() > displayCount) {
            dataStr += "... (共 " + QString::number(signalData.size()) + " 个样本)";
        }
        result += dataStr + "\n";
        
        // 显示数字信号的特征
        if (!signalData.isEmpty()) {
            result += "-----------------------------------\n";
            result += "【信号特征】\n";
            result += "- 数据类型: 双精度浮点数 (double)\n";
            result += "- 数据范围: [-1.0, 1.0]\n";
            result += "- 采样间隔: " + QString::number(1000.0 / sampleFrequency, 'f', 3) + " ms\n";
            result += "- 发送速率: " + QString::number(dataSize * 10) + " 样本/秒\n";
        }
        
        // 显示回复信息
        result += "-----------------------------------\n";
        result += QString("【回复】 已发送ACK到 %1:%2\n").arg(sender.toString()).arg(senderPort);
        result += "===================================\n";
        result += "\n";

        emit dataProcessed(result);
    } catch (const std::exception& e) {
        qCritical() << "Exception in data processing:" << e.what();
        emit dataProcessed(QString("错误: %1\n").arg(e.what()));
    } catch (...) {
        qCritical() << "Unknown exception in data processing";
        emit dataProcessed("错误: 未知异常\n");
    }
}

// TcpReceiverWindow类实现
TcpReceiverWindow::TcpReceiverWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TcpReceiverWindow),
    tcpServer(nullptr),
    workerThread(nullptr),
    dataProcessor(nullptr)
{
    qDebug() << "TcpReceiverWindow constructor started";
    
    try {
        qDebug() << "Creating UI...";
        ui->setupUi(this);
        qDebug() << "UI created successfully";
        
        // 初始化TCP服务器
        qDebug() << "Creating TCP server...";
        tcpServer = new QTcpServer(this);
        qDebug() << "TCP server created";
        
        connect(tcpServer, &QTcpServer::newConnection, this, &TcpReceiverWindow::onNewConnection);
        qDebug() << "Connected newConnection signal";
        
        // 创建数据处理器和工作线程
        qDebug() << "Creating data processor and worker thread...";
        dataProcessor = new DataProcessor();
        workerThread = new QThread();
        dataProcessor->moveToThread(workerThread);
        
        connect(this, &TcpReceiverWindow::destroyed, dataProcessor, &QObject::deleteLater);
        connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);
        connect(dataProcessor, &DataProcessor::dataProcessed, this, &TcpReceiverWindow::onDataProcessed);
        
        workerThread->start();
        qDebug() << "Worker thread started";
        
        // 设置默认值
        qDebug() << "Setting default values...";
        ui->ipLineEdit->setText("127.0.0.1");
        ui->portSpinBox->setValue(8888);
        qDebug() << "Default values set";
        
        qDebug() << "TcpReceiverWindow constructor completed successfully";
    } catch (const std::exception& e) {
        qCritical() << "Exception in TcpReceiverWindow constructor:" << e.what();
        throw;
    } catch (...) {
        qCritical() << "Unknown exception in TcpReceiverWindow constructor";
        throw;
    }
}

TcpReceiverWindow::~TcpReceiverWindow()
{
    if (workerThread) {
        workerThread->quit();
        workerThread->wait();
    }
    
    if (tcpServer) {
        tcpServer->close();
        delete tcpServer;
    }
    
    QMutexLocker locker(&socketMutex);
    for (QTcpSocket *socket : clientSockets) {
        if (socket) {
            socket->close();
            delete socket;
        }
    }
    clientSockets.clear();
    
    delete ui;
    qDebug() << "TCP Receiver Window closed";
}

void TcpReceiverWindow::on_startButton_clicked()
{
    QString ip = ui->ipLineEdit->text();
    int port = ui->portSpinBox->value();
    
    qDebug() << "Starting TCP server on" << ip << ":" << port;
    
    ui->statusLabel->setText("正在启动...");
    ui->startButton->setEnabled(false);
    
    // 使用QTimer在后台执行监听操作，避免阻塞主线程
    QTimer::singleShot(100, this, [=]() {
        QHostAddress address;
        if (ip == "127.0.0.1" || ip == "localhost") {
            address = QHostAddress::LocalHost;
        } else if (ip == "0.0.0.0") {
            address = QHostAddress::Any;
        } else {
            address.setAddress(ip);
        }
        
        qDebug() << "Listening on" << address.toString() << ":" << port;
        
        if (tcpServer->listen(address, port)) {
            ui->statusLabel->setText(QString("已启动，监听 %1:%2").arg(ip).arg(port));
            ui->stopButton->setEnabled(true);
            qDebug() << "TCP server started successfully";
        } else {
            ui->statusLabel->setText("启动失败: " + tcpServer->errorString());
            ui->startButton->setEnabled(true);
            qDebug() << "Failed to start TCP server:" << tcpServer->errorString();
        }
    });
}

void TcpReceiverWindow::on_stopButton_clicked()
{
    QMutexLocker locker(&socketMutex);
    for (QTcpSocket *socket : clientSockets) {
        if (socket) {
            socket->close();
            delete socket;
        }
    }
    clientSockets.clear();
    
    tcpServer->close();
    ui->statusLabel->setText("已停止");
    ui->startButton->setEnabled(true);
    ui->stopButton->setEnabled(false);
    qDebug() << "TCP server stopped";
}

void TcpReceiverWindow::onNewConnection()
{
    QTcpSocket *socket = tcpServer->nextPendingConnection();
    if (socket) {
        QMutexLocker locker(&socketMutex);
        clientSockets.append(socket);
        connect(socket, &QTcpSocket::disconnected, this, &TcpReceiverWindow::onClientDisconnected);
        connect(socket, &QTcpSocket::readyRead, this, &TcpReceiverWindow::onReadyRead);
        
        QHostAddress clientAddress = socket->peerAddress();
        quint16 clientPort = socket->peerPort();
        
        ui->receiveTextEdit->append("===================================");
        ui->receiveTextEdit->append(QString("【连接】 客户端 %1:%2 已连接").arg(clientAddress.toString()).arg(clientPort));
        ui->receiveTextEdit->append("===================================");
        ui->receiveTextEdit->append("");
        
        qDebug() << "Client connected:" << clientAddress.toString() << ":" << clientPort;
    }
}

void TcpReceiverWindow::onClientDisconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (socket) {
        QHostAddress clientAddress = socket->peerAddress();
        quint16 clientPort = socket->peerPort();
        
        QMutexLocker locker(&socketMutex);
        clientSockets.removeOne(socket);
        socket->deleteLater();
        
        ui->receiveTextEdit->append("===================================");
        ui->receiveTextEdit->append(QString("【断开】 客户端 %1:%2 已断开").arg(clientAddress.toString()).arg(clientPort));
        ui->receiveTextEdit->append("===================================");
        ui->receiveTextEdit->append("");
        
        qDebug() << "Client disconnected:" << clientAddress.toString() << ":" << clientPort;
    }
}

void TcpReceiverWindow::onReadyRead()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (socket) {
        QByteArray datagram = socket->readAll();
        // 确保socket在数据处理期间不会被删除
        socket->setParent(nullptr);
        // 在线程中处理数据
        QMetaObject::invokeMethod(dataProcessor, "processData", 
                                 Q_ARG(QTcpSocket*, socket),
                                 Q_ARG(QByteArray, datagram));
    }
}

void TcpReceiverWindow::onDataProcessed(const QString &result)
{
    // 在UI线程中显示结果
    ui->receiveTextEdit->append(result);
}

