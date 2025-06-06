// networkmonitor.h
#pragma once

#include <QObject>
#include <QPair>

#ifdef Q_OS_WIN
#ifdef _WIN32
#include <windows.h>
#endif
#endif

class NetworkMonitor : public QObject {
    Q_OBJECT

public:
    explicit NetworkMonitor(QObject *parent = nullptr);
    
    // 返回下载+上传速率的估算值（单位 MB/s）
    double getNetworkUsage() const;
    
    // 返回上传和下载速率的分离值（单位 bytes/s）
    QPair<quint64, quint64> getNetworkUsageDetailed() const;
    
    // 获取当前活跃的网络接口索引
#ifdef Q_OS_WIN
    DWORD getActiveNetworkInterface() const;
#else
    unsigned long getActiveNetworkInterface() const;
#endif

private:
#ifdef Q_OS_WIN
    mutable quint64 lastRecvBytes, lastSentBytes;
#elif defined(Q_OS_LINUX)
    mutable quint64 lastRecvBytes, lastSentBytes;
#endif
};
