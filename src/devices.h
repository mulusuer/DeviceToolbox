#ifndef DEVICETOOLBOX_DEVICES_H
#define DEVICETOOLBOX_DEVICES_H

#include "device.h"

#include <QList>
#include <QString>

class QTcpSocket;
class QTcpServer;
class QUdpSocket;
class QSerialPort;

// DeviceType：设备类型下拉框的编号，也是参数面板的页号。
enum DeviceType {
    TcpClient  = 0,
    TcpServer  = 1,
    Udp        = 2,
    SerialPort = 3
};

// 功能：按编号造出对应的通信对象。
Device *createDevice(int type, QObject *parent = nullptr);

// 功能：TCP 客户端，主动连一台设备。
class TcpClientDevice : public Device
{
    Q_OBJECT
public:
    explicit TcpClientDevice(QObject *parent = nullptr) : Device(parent) {}
protected:
    QObject *initDevice() override;
    void writeActually(const QByteArray &bytes) override;
    void deinitDevice() override;
private:
    QTcpSocket *m_socket{nullptr};   // m_socket：连设备的 socket，设备线程里造
    QString     m_host;              // m_host：设备地址
    quint16     m_port{0};           // m_port：设备端口
    QString     m_flag;              // m_flag：数据来源标记
};

// 功能：TCP 服务端，等别人连进来，发送时广播给所有客户端。
class TcpServerDevice : public Device
{
    Q_OBJECT
public:
    explicit TcpServerDevice(QObject *parent = nullptr) : Device(parent) {}
protected:
    QObject *initDevice() override;
    void writeActually(const QByteArray &bytes) override;
    void deinitDevice() override;
private:
    QTcpServer         *m_server{nullptr};  // m_server：监听端口的服务端
    QList<QTcpSocket *> m_clients;          // m_clients：当前连着的客户端
    QString             m_flag;             // m_flag：数据来源标记
};

// 功能：UDP，无连接，往指定地址发、从任意地址收。
class UdpDevice : public Device
{
    Q_OBJECT
public:
    explicit UdpDevice(QObject *parent = nullptr) : Device(parent) {}
protected:
    QObject *initDevice() override;
    void writeActually(const QByteArray &bytes) override;
private:
    QUdpSocket *m_udp{nullptr};      // m_udp：UDP socket，设备线程里造
    QString     m_remoteHost;        // m_remoteHost：发送目标地址
    quint16     m_remotePort{0};     // m_remotePort：发送目标端口
    QString     m_flag;              // m_flag：数据来源标记
};

// 功能：串口，按界面约定的参数打开端口收发字节。
class SerialDevice : public Device
{
    Q_OBJECT
public:
    explicit SerialDevice(QObject *parent = nullptr) : Device(parent) {}
protected:
    QObject *initDevice() override;
    void writeActually(const QByteArray &bytes) override;
    void deinitDevice() override;
private:
    QSerialPort *m_serial{nullptr};  // m_serial：串口对象，设备线程里造
    QString      m_flag;             // m_flag：数据来源标记
};

#endif // DEVICETOOLBOX_DEVICES_H
