#include "devices.h"

#include <QHostAddress>
#include <QSerialPort>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>

// 功能：按编号造出对应的通信对象。
Device *createDevice(int type, QObject *parent)
{
    switch (type) {
    case TcpClient:  return new TcpClientDevice(parent);
    case TcpServer:  return new TcpServerDevice(parent);
    case Udp:        return new UdpDevice(parent);
    case SerialPort: return new SerialDevice(parent);
    default:         return nullptr;
    }
}

// 功能：连上设备，把收数据和断线的信号接到设备线程。
QObject *TcpClientDevice::initDevice()
{
    QVariantMap p = parameters();
    m_host = p.value("host", "127.0.0.1").toString();
    m_port = quint16(p.value("port", 1502).toInt());

    QTcpSocket *socket = new QTcpSocket();   // socket：设备线程里造的连接对象
    m_socket = socket;

    connect(socket, &QTcpSocket::readyRead, socket, [this]() {
        emit bytesRead(m_socket->readAll(), m_flag);
    });
    connect(socket, &QTcpSocket::disconnected, socket, [this]() {
        if (!isInterruptionRequested()) emit closed();
    });
    connect(socket, &QTcpSocket::errorOccurred, socket, [this]() {
        emit errorOccurred(m_socket->errorString());
    });

    socket->connectToHost(m_host, m_port);
    if (!socket->waitForConnected(3000)) {
        emit errorOccurred(socket->errorString());
        delete socket;
        m_socket = nullptr;
        return nullptr;
    }

    m_flag = QString("%1:%2").arg(m_host).arg(m_port);
    emit opened();
    return socket;
}

// 功能：把字节写进 socket 并回报已发送。
void TcpClientDevice::writeActually(const QByteArray &bytes)
{
    m_socket->write(bytes);
    emit bytesWritten(bytes, m_flag);
}

// 功能：关掉连接。
void TcpClientDevice::deinitDevice()
{
    if (m_socket) m_socket->close();
}

// 功能：监听端口，每来一个客户端就接好收数据信号。
QObject *TcpServerDevice::initDevice()
{
    int port = parameters().value("port", 1503).toInt();

    QTcpServer *server = new QTcpServer();   // server：设备线程里造的监听对象
    m_server = server;

    connect(server, &QTcpServer::newConnection, server, [this]() {
        while (m_server->hasPendingConnections()) {
            QTcpSocket *client = m_server->nextPendingConnection();  // client：新连进来的 socket
            m_clients.append(client);
            connect(client, &QTcpSocket::readyRead, client, [this, client]() {
                QString from = client->peerAddress().toString();   // from：客户端地址
                if (from.startsWith("::ffff:")) from = from.mid(7);
                from += ":" + QString::number(client->peerPort());
                emit bytesRead(client->readAll(), from);
            });
            connect(client, &QTcpSocket::disconnected, client, [this, client]() {
                m_clients.removeOne(client);
                client->deleteLater();
            });
        }
    });

    if (!server->listen(QHostAddress::Any, quint16(port))) {
        emit errorOccurred(server->errorString());
        delete server;
        m_server = nullptr;
        return nullptr;
    }

    m_flag = "0.0.0.0:" + QString::number(port);
    emit opened();
    return server;
}

// 功能：把字节广播给所有在线客户端。
void TcpServerDevice::writeActually(const QByteArray &bytes)
{
    for (QTcpSocket *client : m_clients) {
        if (client->state() == QAbstractSocket::ConnectedState) client->write(bytes);
    }
    emit bytesWritten(bytes, m_flag);
}

// 功能：关掉监听。
void TcpServerDevice::deinitDevice()
{
    if (m_server) m_server->close();
    m_clients.clear();
}

// 功能：绑定本地端口，收数据报并回报发送方地址。
QObject *UdpDevice::initDevice()
{
    QVariantMap p = parameters();
    m_remoteHost = p.value("host", "127.0.0.1").toString();
    m_remotePort = quint16(p.value("port", 1502).toInt());
    int localPort = p.value("localPort", 1504).toInt();

    QUdpSocket *udp = new QUdpSocket();      // udp：设备线程里造的 UDP socket
    m_udp = udp;
    if (!udp->bind(QHostAddress::Any, quint16(localPort))) {
        emit errorOccurred(udp->errorString());
        delete udp;
        m_udp = nullptr;
        return nullptr;
    }

    connect(udp, &QUdpSocket::readyRead, udp, [this]() {
        while (m_udp->hasPendingDatagrams()) {
            QByteArray datagram;             // datagram：一个完整数据报
            datagram.resize(int(m_udp->pendingDatagramSize()));
            QHostAddress sender;             // sender：数据报来源地址
            quint16 senderPort{0};           // senderPort：数据报来源端口
            m_udp->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
            QString from = sender.toString();   // from：来源地址文字
            if (from.startsWith("::ffff:")) from = from.mid(7);
            emit bytesRead(datagram, from + ":" + QString::number(senderPort));
        }
    });

    m_flag = m_remoteHost + ":" + QString::number(m_remotePort);
    emit opened();
    return udp;
}

// 功能：把字节发到目标地址。
void UdpDevice::writeActually(const QByteArray &bytes)
{
    m_udp->writeDatagram(bytes, QHostAddress(m_remoteHost), m_remotePort);
    emit bytesWritten(bytes, m_flag);
}

// 功能：按界面约定的参数打开串口，把收数据信号接到设备线程。
QObject *SerialDevice::initDevice()
{
    QVariantMap p = parameters();

    QSerialPort *serial = new QSerialPort();   // serial：设备线程里造的串口对象
    m_serial = serial;
    serial->setPortName(p.value("portName").toString());
    serial->setBaudRate(p.value("baudRate", 9600).toInt());
    serial->setDataBits(static_cast<QSerialPort::DataBits>(p.value("dataBits", 8).toInt()));
    serial->setParity(static_cast<QSerialPort::Parity>(p.value("parity", 0).toInt()));
    serial->setStopBits(static_cast<QSerialPort::StopBits>(p.value("stopBits", 1).toInt()));
    serial->setFlowControl(static_cast<QSerialPort::FlowControl>(p.value("flowControl", 0).toInt()));

    if (!serial->open(QIODevice::ReadWrite)) {
        emit errorOccurred(serial->errorString());
        delete serial;
        m_serial = nullptr;
        return nullptr;
    }

    connect(serial, &QSerialPort::readyRead, serial, [this]() {
        emit bytesRead(m_serial->readAll(), m_flag);
    });

    m_flag = p.value("portName").toString();
    emit opened();
    return serial;
}

// 功能：把字节写进串口并回报已发送。
void SerialDevice::writeActually(const QByteArray &bytes)
{
    m_serial->write(bytes);
    emit bytesWritten(bytes, m_flag);
}

// 功能：关掉串口。
void SerialDevice::deinitDevice()
{
    if (m_serial) m_serial->close();
}
