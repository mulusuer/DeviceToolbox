#include "modbusmaster.h"

#include <QDateTime>

// 功能：建 Modbus 主站协议对象。
ModbusMaster::ModbusMaster(QObject *parent)
    : QObject(parent)
{
}

// 功能：拼一条「读保持寄存器」请求帧（12 字节）。
QByteArray ModbusMaster::buildReadRequest(quint8 unit, quint16 start, quint16 count) const
{
    QByteArray bytes;
    auto put2 = [&bytes](quint16 v) {
        bytes.append(char((v >> 8) & 0xFF));   // 高字节
        bytes.append(char(v & 0xFF));          // 低字节
    };

    put2(1);       // 事务 ID：第一版固定 1，后面按序递增也行
    put2(0);       // 协议 ID：Modbus TCP 规定为 0
    put2(6);       // 长度：后面还有 6 字节
    bytes.append(char(unit));                  // 从站号
    bytes.append(char(0x03));                  // 功能码：读保持寄存器
    put2(start);                               // 起始地址
    put2(count);                               // 读几个
    return bytes;
}

// 功能：解析一帧应答，抠出寄存器值并按倍数换算。
bool ModbusMaster::parseResponse(const QByteArray &frame, QVariantMap &outSample, QString &outError) const
{
    outSample.clear();
    outError.clear();

    if (frame.size() < 9) {
        outError = "应答太短";
        return false;
    }
    auto u8 = [&frame](int i) { return (unsigned char) frame[i]; };  // u8：按无符号取字节

    // MBAP 头：事务ID(2) 协议ID(2) 长度(2) 从站(1)
    if (u8(7) & 0x80) {                        // 功能码最高位为 1 = 异常应答
        outError = QString("设备返回异常，异常码 %1").arg(u8(8));
        return false;
    }
    if (u8(2) != 0 || u8(3) != 0) {
        outError = "不是 Modbus TCP 帧";
        return false;
    }
    int length = (u8(4) << 8) | u8(5);         // length：长度字段，后面还有几字节
    if (length != frame.size() - 6) {
        outError = "长度字段对不上";
        return false;
    }
    if (u8(7) != 0x03) {
        outError = QString("功能码不是 03，实际 %1").arg(u8(7));
        return false;
    }

    int byteCount = u8(8);                     // byteCount：数据区字节数
    if (byteCount < 8 || byteCount % 2 != 0 || 9 + byteCount != frame.size()) {
        outError = "数据区长度不对";
        return false;
    }

    // 假设备约定：4 个寄存器依次是 温度×10、电流×10、电压×10、状态
    auto reg = [&](int i) {
        int p = 9 + i * 2;
        return (u8(p) << 8) | u8(p + 1);       // 大端：高字节在前
    };

    outSample["temp"] = reg(0) / 10.0;
    outSample["current"] = reg(1) / 10.0;
    outSample["voltage"] = reg(2) / 10.0;
    outSample["status"] = reg(3);
    outSample["raw"] = QString::fromLatin1(frame.toHex(' '));
    return true;
}

// 功能：设温度告警阈值。
void ModbusMaster::setTempLimit(double limit)
{
    m_tempLimit = limit;
}

// 功能：收到一帧后的统一入口，解析成功发 sampleReady，失败发 protocolError。
void ModbusMaster::onFrame(const QByteArray &frame)
{
    QVariantMap sample;
    QString error;
    if (!parseResponse(frame, sample, error)) {
        emit protocolError(error);
        return;
    }

    qint64 ts = QDateTime::currentMSecsSinceEpoch();   // ts：收到这一帧的本机时间
    emit sampleReady(ts, sample);

    double temp = sample.value("temp").toDouble();
    if (temp > m_tempLimit) {
        emit alarm(QString("温度 %1 超过阈值 %2")
                   .arg(QString::number(temp, 'f', 1))
                   .arg(QString::number(m_tempLimit, 'f', 1)));
    }
}
