// memorymonitor.cpp
#include "src/include/monitor/memorymonitor.h"
#ifdef Q_OS_WIN
#include <windows.h>
#elif defined(Q_OS_LINUX)
#include <fstream>
#include <string>
#endif

MemoryMonitor::MemoryMonitor(QObject *parent) : QObject(parent) {
#ifdef Q_OS_WIN
    // 初始化内存状态结构
    memoryStatus.dwLength = sizeof(MEMORYSTATUSEX);
#endif
}

double MemoryMonitor::getMemoryUsage() const {
#ifdef Q_OS_WIN
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    if (GlobalMemoryStatusEx(&memStatus)) {
        return 100.0 - memStatus.ullAvailPhys * 100.0 / memStatus.ullTotalPhys;
    }
    return 0.0;
#elif defined(Q_OS_LINUX)
    std::ifstream file("/proc/meminfo");
    std::string line;
    long totalMem = 0, freeMem = 0, buffers = 0, cached = 0;
    while (std::getline(file, line)) {
        if (line.find("MemTotal:") == 0)
            sscanf(line.c_str(), "MemTotal: %ld kB", &totalMem);
        else if (line.find("MemFree:") == 0)
            sscanf(line.c_str(), "MemFree: %ld kB", &freeMem);
        else if (line.find("Buffers:") == 0)
            sscanf(line.c_str(), "Buffers: %ld kB", &buffers);
        else if (line.find("Cached:") == 0)
            sscanf(line.c_str(), "Cached: %ld kB", &cached);
    }
    long used = totalMem - freeMem - buffers - cached;
    if (totalMem == 0) return 0.0;
    return used * 100.0 / totalMem;
#endif
}

quint64 MemoryMonitor::getTotalMemory() const {
#ifdef Q_OS_WIN
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    if (GlobalMemoryStatusEx(&memStatus)) {
        return memStatus.ullTotalPhys;
    }
    return 0;
#elif defined(Q_OS_LINUX)
    std::ifstream file("/proc/meminfo");
    std::string line;
    long totalMem = 0;
    while (std::getline(file, line)) {
        if (line.find("MemTotal:") == 0) {
            sscanf(line.c_str(), "MemTotal: %ld kB", &totalMem);
            break;
        }
    }
    return static_cast<quint64>(totalMem) * 1024; // 转换为字节
#endif
}

quint64 MemoryMonitor::getUsedMemory() const {
#ifdef Q_OS_WIN
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    if (GlobalMemoryStatusEx(&memStatus)) {
        return memStatus.ullTotalPhys - memStatus.ullAvailPhys;
    }
    return 0;
#elif defined(Q_OS_LINUX)
    std::ifstream file("/proc/meminfo");
    std::string line;
    long totalMem = 0, freeMem = 0, buffers = 0, cached = 0;
    while (std::getline(file, line)) {
        if (line.find("MemTotal:") == 0)
            sscanf(line.c_str(), "MemTotal: %ld kB", &totalMem);
        else if (line.find("MemFree:") == 0)
            sscanf(line.c_str(), "MemFree: %ld kB", &freeMem);
        else if (line.find("Buffers:") == 0)
            sscanf(line.c_str(), "Buffers: %ld kB", &buffers);
        else if (line.find("Cached:") == 0)
            sscanf(line.c_str(), "Cached: %ld kB", &cached);
    }
    long used = totalMem - freeMem - buffers - cached;
    return static_cast<quint64>(used) * 1024; // 转换为字节
#endif
}
