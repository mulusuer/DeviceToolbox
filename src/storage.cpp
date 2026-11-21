#include "storage.h"

#include <QSqlError>
#include <QSqlQuery>

// 功能：打开数据库并建表。
bool Storage::open(const QString &dbFilePath, QString &outError)
{
    if (m_db.isOpen()) return true;

    m_db = QSqlDatabase::addDatabase("QSQLITE", "device_toolbox");
    m_db.setDatabaseName(dbFilePath);
    if (!m_db.open()) {
        outError = m_db.lastError().text();
        return false;
    }

    QSqlQuery query(m_db);
    if (!query.exec("CREATE TABLE IF NOT EXISTS records ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "ts INTEGER NOT NULL, "
                    "direction TEXT, protocol TEXT, peer TEXT, "
                    "size INTEGER, payload_hex TEXT)")) {
        outError = query.lastError().text();
        return false;
    }
    if (!query.exec("CREATE TABLE IF NOT EXISTS samples ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "ts INTEGER NOT NULL, "
                    "temp REAL, current_ma REAL, voltage REAL, status INTEGER)")) {
        outError = query.lastError().text();
        return false;
    }
    return true;
}

// 功能：关闭数据库。
void Storage::close()
{
    m_db.close();
}

// 功能：插一条收发记录。
bool Storage::log(qint64 tsMs, const QString &direction, const QString &protocol,
                  const QString &peer, const QByteArray &payload)
{
    if (!m_db.isOpen()) return false;

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO records (ts, direction, protocol, peer, size, payload_hex) "
                  "VALUES (?, ?, ?, ?, ?, ?)");
    query.addBindValue(tsMs);
    query.addBindValue(direction);
    query.addBindValue(protocol);
    query.addBindValue(peer);
    query.addBindValue(int(payload.size()));
    query.addBindValue(QString::fromLatin1(payload.toHex(' ')));
    return query.exec();
}

// 功能：插一条 Modbus 采样值。
bool Storage::insertSample(qint64 tsMs, double temp, double current, double voltage, int status)
{
    if (!m_db.isOpen()) return false;
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO samples (ts, temp, current_ma, voltage, status) "
                  "VALUES (?, ?, ?, ?, ?)");
    query.addBindValue(tsMs);
    query.addBindValue(temp);
    query.addBindValue(current);
    query.addBindValue(voltage);
    query.addBindValue(status);
    return query.exec();
}

// 功能：返回采样条数，失败返回 -1。
int Storage::sampleCount() const
{
    if (!m_db.isOpen()) return -1;
    QSqlQuery query(m_db);
    if (!query.exec("SELECT COUNT(*) FROM samples")) return -1;
    if (!query.next()) return -1;
    return query.value(0).toInt();
}

// 功能：返回记录条数，失败返回 -1。
int Storage::recordCount() const
{
    if (!m_db.isOpen()) return -1;
    QSqlQuery query(m_db);
    if (!query.exec("SELECT COUNT(*) FROM records")) return -1;
    if (!query.next()) return -1;
    return query.value(0).toInt();
}

// 功能：删掉所有记录。
bool Storage::clear()
{
    if (!m_db.isOpen()) return false;
    QSqlQuery query(m_db);
    return query.exec("DELETE FROM records");
}
