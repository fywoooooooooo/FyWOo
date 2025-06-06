// exporter.cpp
#include "src/include/storage/exporter.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QTextStream>
#include <QDebug>

Exporter::Exporter(QObject *parent) : QObject(parent) {}

bool Exporter::exportToCsv(const QString &dbPath, const QString &csvPath,
                           const QDateTime &from, const QDateTime &to) {
    bool result = false;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "export_connection");
        db.setDatabaseName(dbPath);
        if (!db.open()) {
            qWarning() << "无法打开数据库:" << db.lastError();
            return false;
        }

        QString queryStr = "SELECT type, value, timestamp FROM samples";
        if (from.isValid() && to.isValid()) {
            queryStr += " WHERE timestamp BETWEEN '" + from.toString(Qt::ISODate) + "' AND '" + to.toString(Qt::ISODate) + "'";
        }

        QFile file(csvPath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qWarning() << "无法写入文件:" << csvPath;
            db.close();
            return false;
        }

        QTextStream out(&file);
        out << "type,value,timestamp\n";

        {
            QSqlQuery query(db);
            if (!query.exec(queryStr)) {
                qWarning() << "查询失败:" << query.lastError();
                file.close();
                db.close();
                return false;
            }
            while (query.next()) {
                out << query.value(0).toString() << ","
                    << QString::number(query.value(1).toDouble(), 'f', 4) << ","
                    << query.value(2).toString() << "\n";
            }
        }
        file.close();
        db.close();
        result = true;
    }
    QSqlDatabase::removeDatabase("export_connection");
    return result;
}
