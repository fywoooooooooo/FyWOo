// diskmonitor.cpp
#include "src/include/monitor/diskmonitor.h"
#ifdef Q_OS_WIN
#include <windows.h>
#include <winioctl.h>
#elif defined(Q_OS_LINUX)
#include <fstream>
#include <sstream>
#endif

DiskMonitor::DiskMonitor(QObject *parent) : QObject(parent) {
#ifdef Q_OS_WIN
    lastReadBytes = lastWriteBytes = 0;
#elif defined(Q_OS_LINUX)
    lastReadSectors = lastWriteSectors = 0;
#endif
}
  
double DiskMonitor::getDiskIO() const {
#ifdef Q_OS_WIN
    static ULARGE_INTEGER idleTime, kernelTime, userTime;
    static FILETIME idle, kernel, user;
    GetSystemTimes(&idle, &kernel, &user);
    return 0.0; // Windows实现通常较复杂，可考虑查询性能计数器
#elif defined(Q_OS_LINUX)
    std::ifstream file("/proc/diskstats");
    std::string line;
    quint64 totalRead = 0, totalWrite = 0;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string dev;
        quint64 major, minor, reads, rdMerged, rdSectors, rdTime;
        quint64 writes, wrMerged, wrSectors, wrTime;
        iss >> major >> minor >> dev;
        iss >> reads >> rdMerged >> rdSectors >> rdTime;
        iss >> writes >> wrMerged >> wrSectors >> wrTime;

        if (dev.find("sd") == 0 || dev.find("nvme") == 0) {
            totalRead += rdSectors;
            totalWrite += wrSectors;
        }
    }

    quint64 readDiff = totalRead - lastReadSectors;
    quint64 writeDiff = totalWrite - lastWriteSectors;
    lastReadSectors = totalRead;
    lastWriteSectors = totalWrite;

    return static_cast<double>(readDiff + writeDiff) / 2048.0; // 假设扇区512B -> MB/s
#endif
    return 0.0;
}
