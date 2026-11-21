#include "frameparser.h"

// 功能：换切法并清空残留。
void FrameParser::setMode(Mode mode)
{
    m_mode = mode;
    reset();
}

// 功能：清空没凑齐的零头。
void FrameParser::reset()
{
    m_cache.clear();
}

// 功能：喂入本次收到的字节，返回切好的整帧列表。
QList<QByteArray> FrameParser::feed(const QByteArray &newBytes)
{
    QList<QByteArray> frames;                // frames：这次切出来的整帧
    if (newBytes.isEmpty()) return frames;

    if (m_mode == Mode::Raw) {
        frames.append(newBytes);
        return frames;
    }

    m_cache.append(newBytes);

    if (m_mode == Mode::Line) {
        while (true) {
            int pos = m_cache.indexOf('\n');  // pos：第一个换行符的位置
            if (pos < 0) break;
            frames.append(m_cache.left(pos));
            m_cache.remove(0, pos + 1);
        }
    } else {
        while (m_cache.size() >= 6) {
            int total = ((unsigned char) m_cache[4] << 8)
                        + (unsigned char) m_cache[5] + 6;   // total：整帧长度 = 6 + 长度字段
            if (total <= 6 || m_cache.size() < total) break;
            frames.append(m_cache.left(total));
            m_cache.remove(0, total);
        }
    }

    // 切完之后还剩下的零头，超过上限才整块吐出，避免切分被跳过、缓存无限涨。
    if (m_cache.size() > 4096) {
        frames.append(m_cache);
        m_cache.clear();
    }
    return frames;
}
