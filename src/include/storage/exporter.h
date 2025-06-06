#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>

class Exporter : public QObject {
    Q_OBJECT

public:
    explicit Exporter(QObject *parent = nullptr);
    bool exportToCsv(const QString &dbPath, const QString &csvPath = "Data/export.csv",
                     const QDateTime &from = QDateTime(), const QDateTime &to = QDateTime());
};
