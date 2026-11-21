#ifndef DEVICETOOLBOX_FRAMEPARSER_H
#define DEVICETOOLBOX_FRAMEPARSER_H

#include <QByteArray>
#include <QList>

// 功能：按选中的格式把连续字节流切成一帧一帧。
class FrameParser
{
public:
    // Mode：帧的切法，与具体通信方式无关
    enum class Mode {
        Raw = 0,        // 不切，来多少显示多少
        Line = 1,       // 按换行符切
        ModbusTcp = 2   // 按 MBAP 头里的长度字段切
    };

    // 功能：换切法并清空残留。
    void setMode(Mode mode);
    // 功能：清空没凑齐的零头。
    void reset();
    // 功能：喂入本次收到的字节，返回切好的整帧列表。
    QList<QByteArray> feed(const QByteArray &newBytes);

private:
    Mode        m_mode{Mode::Raw};   // m_mode：当前切法
    QByteArray  m_cache;             // m_cache：没凑齐一帧的残留字节
};

#endif // DEVICETOOLBOX_FRAMEPARSER_H
