#ifndef DEVICETOOLBOX_MODBUSMASTER_H
#define DEVICETOOLBOX_MODBUSMASTER_H

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QVariantMap>

// 功能：Modbus TCP 主站协议层，拼读请求、解析应答、换算工程值、判断告警。
class ModbusMaster : public QObject
{
    Q_OBJECT
public:
    explicit ModbusMaster(QObject *parent = nullptr);

    // 功能：拼一条「读保持寄存器」请求帧。unit 从站号，start 起始地址，count 读几个
    QByteArray buildReadRequest(quint8 unit, quint16 start, quint16 count) const;

    // 功能：解析一帧应答，成功把采样值放进 outSample；失败写 outError
    bool parseResponse(const QByteArray &frame, QVariantMap &outSample, QString &outError) const;

    // 功能：设温度告警阈值，超过就发 alarm
    void setTempLimit(double limit);

public slots:
    // 功能：收到一帧后的统一入口，解析成功发 sampleReady，失败发 protocolError
    void onFrame(const QByteArray &frame);

signals:
    // sampleReady：拿到一次采样，tsMs 本机时间戳，sample 含 temp/current/voltage/status
    void sampleReady(qint64 tsMs, const QVariantMap &sample);
    // alarm：温度超过阈值
    void alarm(const QString &text);
    // protocolError：解析失败原因
    void protocolError(const QString &text);

private:
    double m_tempLimit{30.0};   // m_tempLimit：温度告警阈值
};

#endif // DEVICETOOLBOX_MODBUSMASTER_H
