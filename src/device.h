#ifndef DEVICETOOLBOX_DEVICE_H
#define DEVICETOOLBOX_DEVICE_H

#include <QMutex>
#include <QByteArray>
#include <QString>
#include <QThread>
#include <QVariantMap>

// 功能：所有通信方式的基类，管线程、开关和统一信号。
class Device : public QThread
{
    Q_OBJECT
public:
    explicit Device(QObject *parent = nullptr);
    ~Device() override;

    // 功能：记下界面参数并启动线程。
    void openDevice(const QVariantMap &parameters);
    // 功能：停事件循环、等线程退出。
    void closeDevice();
    // 功能：从界面线程请求发送一帧，排队到设备线程。
    void writeBytes(const QByteArray &bytes);

signals:
    // bytesRead：收到一帧，flag 标记数据来自谁
    void bytesRead(const QByteArray &bytes, const QString &flag);
    // bytesWritten：发出一帧
    void bytesWritten(const QByteArray &bytes, const QString &flag);
    // opened：通信对象就绪
    void opened();
    // closed：连接关闭
    void closed();
    // errorOccurred：出错原因
    void errorOccurred(const QString &text);
    // invokeWriteBytes：内部排队用，界面线程只喊这一声
    void invokeWriteBytes(const QByteArray &bytes);

protected:
    void run() override;
    // 功能：在设备线程里造通信对象并连好信号，返回该对象（子类填）。
    virtual QObject *initDevice() = 0;
    // 功能：在设备线程里真正写字节（子类填）。
    virtual void writeActually(const QByteArray &bytes) = 0;
    // 功能：在设备线程里收尾（子类填）。
    virtual void deinitDevice() {}
    // 功能：取界面打包的参数。
    QVariantMap parameters();

private:
    QVariantMap m_parameters;   // m_parameters：界面打包的设备参数
    QMutex      m_mutex;        // m_mutex：护住 m_parameters
};

#endif // DEVICETOOLBOX_DEVICE_H
