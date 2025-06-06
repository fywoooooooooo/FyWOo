// cpumonitor.cpp
#include "src/include/monitor/cpumonitor.h"
#ifdef Q_OS_WIN
#include <windows.h>
#include <pdh.h>
#include <pdhmsg.h>
#pragma comment(lib, "pdh.lib")
#elif defined(Q_OS_LINUX)
#include <fstream>
#include <sstream>
#endif

CpuMonitor::CpuMonitor(QObject *parent) : QObject(parent) {
#ifdef Q_OS_WIN
    initPdh();
#elif defined(Q_OS_LINUX)
    lastTotalUser = lastTotalUserLow = lastTotalSys = lastTotalIdle = 0;
    initialized = false;
#endif
}

#ifdef Q_OS_WIN
void CpuMonitor::initPdh() {
    PdhOpenQuery(NULL, 0, (PDH_HQUERY *)&queryHandle);
    PdhAddCounter((PDH_HQUERY)queryHandle, L"\\Processor(_Total)\\% Processor Time", 0, (PDH_HCOUNTER *)&counterHandle);
    PdhCollectQueryData((PDH_HQUERY)queryHandle);
}

void CpuMonitor::cleanupPdh() {
    PdhCloseQuery((PDH_HQUERY)queryHandle);
}
#endif

double CpuMonitor::getCpuUsage() const {
#ifdef Q_OS_WIN
    PDH_FMT_COUNTERVALUE counterVal;
    PdhCollectQueryData((PDH_HQUERY)queryHandle);
    PdhGetFormattedCounterValue((PDH_HCOUNTER)counterHandle, PDH_FMT_DOUBLE, NULL, &counterVal);
    return counterVal.doubleValue;
#elif defined(Q_OS_LINUX)
    std::ifstream file("/proc/stat");
    std::string line;
    std::getline(file, line);
    std::istringstream iss(line);

    std::string cpu;
    quint64 user, nice, system, idle;
    iss >> cpu >> user >> nice >> system >> idle;

    quint64 totalUser = user;
    quint64 totalUserLow = nice;
    quint64 totalSys = system;
    quint64 totalIdle = idle;

    if (!initialized) {
        lastTotalUser = totalUser;
        lastTotalUserLow = totalUserLow;
        lastTotalSys = totalSys;
        lastTotalIdle = totalIdle;
        initialized = true;
        return 0.0;
    }

    quint64 diffUser = totalUser - lastTotalUser;
    quint64 diffUserLow = totalUserLow - lastTotalUserLow;
    quint64 diffSys = totalSys - lastTotalSys;
    quint64 diffIdle = totalIdle - lastTotalIdle;

    quint64 total = diffUser + diffUserLow + diffSys + diffIdle;

    lastTotalUser = totalUser;
    lastTotalUserLow = totalUserLow;
    lastTotalSys = totalSys;
    lastTotalIdle = totalIdle;

    if (total == 0) return 0.0;
    return (double)(diffUser + diffUserLow + diffSys) / total * 100.0;
#endif
    return 0.0;
}
