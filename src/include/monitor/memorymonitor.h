// memorymonitor.h
#pragma once

#include <QObject>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

class MemoryMonitor : public QObject {
    Q_OBJECT

public:
    explicit MemoryMonitor(QObject *parent = nullptr);
    double getMemoryUsage() const;
    quint64 getTotalMemory() const;
    quint64 getUsedMemory() const;
    
    // 内存泄漏检测功能
#ifdef _MSC_VER
    void enableMemoryLeakCheck();
#endif

private:
#ifdef Q_OS_WIN
    MEMORYSTATUSEX memoryStatus;
#elif defined(Q_OS_LINUX)
         // 无需缓存成员变量
#endif
};
