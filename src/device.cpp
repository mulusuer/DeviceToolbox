#include "device.h"

#include <QMutexLocker>

Device::Device(QObject *parent)
    : QThread(parent)
{
}

Device::~Device()
{
    closeDevice();
}

// 功能：记下界面参数并启动线程。
void Device::openDevice(const QVariantMap &parameters)
{
    closeDevice();
    {
        QMutexLocker locker(&m_mutex);   // locker：退出作用域自动解锁
        m_parameters = parameters;
    }
    start();
}

// 功能：停事件循环、等线程退出。
void Device::closeDevice()
{
    if (!isRunning()) return;
    requestInterruption();
    quit();
    wait();
}

// 功能：从界面线程请求发送一帧，排队到设备线程。
void Device::writeBytes(const QByteArray &bytes)
{
    if (isRunning()) emit invokeWriteBytes(bytes);
}

// 功能：取界面打包的参数。
QVariantMap Device::parameters()
{
    QMutexLocker locker(&m_mutex);       // locker：退出作用域自动解锁
    return m_parameters;
}

// 功能：开线程建通信对象、接好写请求、进事件循环、退出时收尾。
void Device::run()
{
    QObject *obj = initDevice();         // obj：设备线程里造出的通信对象
    if (!obj) {
        emit errorOccurred("设备初始化失败，请检查参数或端口是否被占用");
        emit closed();
        return;
    }

    connect(this, &Device::invokeWriteBytes, obj, [this](const QByteArray &bytes) {
        writeActually(bytes);
    });

    if (isInterruptionRequested()) {
        deinitDevice();
        delete obj;
        emit closed();
        return;
    }

    exec();

    deinitDevice();
    delete obj;
    emit closed();
}
