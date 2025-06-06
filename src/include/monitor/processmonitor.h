// processmonitor.h
#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include "../common/processinfo.h"

class ProcessMonitor : public QObject {
    Q_OBJECT

public:
    explicit ProcessMonitor(QObject *parent = nullptr);
    QList<ProcessInfo> getTopProcesses(int maxCount = 10);
};
