// cpumonitor.h
#pragma once

#include <QObject>

class CpuMonitor : public QObject {
    Q_OBJECT

public:
    explicit CpuMonitor(QObject *parent = nullptr);
    double getCpuUsage() const;

private:
#ifdef Q_OS_WIN
         // Windows-specific handles and data
    void initPdh();
    void cleanupPdh();
    void *queryHandle;
    void *counterHandle;
#elif defined(Q_OS_LINUX)
    quint64 lastTotalUser, lastTotalUserLow, lastTotalSys, lastTotalIdle;
    bool initialized;
#endif
};
