#ifndef PROCESSINFO_H
#define PROCESSINFO_H

#include <QString>

struct ProcessInfo {
    quint64 pid;
    QString name;
    double usage;       // CPU or Memory usage (generic)
    QString usageString; // Formatted usage string
    
    // 兼容ProcessMonitor类中使用的字段
    double cpuPercent;  // CPU usage percentage
    double memoryMB;    // Memory usage in MB
};

#endif // PROCESSINFO_H