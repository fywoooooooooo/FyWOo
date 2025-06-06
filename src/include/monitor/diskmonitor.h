// diskmonitor.h
#pragma once

#include <QObject>

class DiskMonitor : public QObject {
    Q_OBJECT

public:
    explicit DiskMonitor(QObject *parent = nullptr);
    double getDiskIO() const; // 返回某时间段内读写变化量的估计值

private:
#ifdef Q_OS_WIN
    quint64 lastReadBytes, lastWriteBytes;
#elif defined(Q_OS_LINUX)
    quint64 lastReadSectors, lastWriteSectors;
#endif
};
