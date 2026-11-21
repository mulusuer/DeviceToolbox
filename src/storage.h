#ifndef DEVICETOOLBOX_STORAGE_H
#define DEVICETOOLBOX_STORAGE_H

#include <QByteArray>
#include <QSqlDatabase>
#include <QString>

// 功能：把每一帧收发记录写进 SQLite，供事后检索。
class Storage
{
public:
    // 功能：打开数据库并建表。
    bool open(const QString &dbFilePath, QString &outError);
    // 功能：关闭数据库。
    void close();
    // 功能：插一条收发记录。
    bool log(qint64 tsMs, const QString &direction, const QString &protocol,
             const QString &peer, const QByteArray &payload);
    // 功能：插一条 Modbus 采样值。
    bool insertSample(qint64 tsMs, double temp, double current, double voltage, int status);
    // 功能：返回收发记录条数，失败返回 -1。
    int recordCount() const;
    // 功能：返回采样条数，失败返回 -1。
    int sampleCount() const;
    // 功能：删掉所有记录。
    bool clear();

private:
    QSqlDatabase m_db;   // m_db：SQLite 连接
};

#endif // DEVICETOOLBOX_STORAGE_H
