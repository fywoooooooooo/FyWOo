// networkmonitor.cpp
#include "src/include/monitor/networkmonitor.h"
#ifdef Q_OS_WIN
#include <iphlpapi.h>
#ifdef _MSC_VER
#pragma comment(lib, "iphlpapi.lib")
#endif
#elif defined(Q_OS_LINUX)
#include <fstream>
#include <sstream>
#endif

NetworkMonitor::NetworkMonitor(QObject *parent) : QObject(parent) {
    lastRecvBytes = lastSentBytes = 0;
}

double NetworkMonitor::getNetworkUsage() const {
    QPair<quint64, quint64> usage = getNetworkUsageDetailed();
    return static_cast<double>(usage.first + usage.second) / (1024.0 * 1024.0); // MB
}

QPair<quint64, quint64> NetworkMonitor::getNetworkUsageDetailed() const {
#ifdef Q_OS_WIN
    PMIB_IFTABLE pIfTable;
    DWORD dwSize = sizeof(MIB_IFTABLE);
    pIfTable = (PMIB_IFTABLE)malloc(dwSize);
    
    if (GetIfTable(pIfTable, &dwSize, TRUE) == ERROR_INSUFFICIENT_BUFFER) {
        free(pIfTable);
        pIfTable = (PMIB_IFTABLE)malloc(dwSize);
    }
    
    if (GetIfTable(pIfTable, &dwSize, TRUE) == NO_ERROR) {
        quint64 totalRecv = 0, totalSent = 0;
        
        // 获取活跃网络接口
        DWORD activeInterface = getActiveNetworkInterface();
        
        // 如果找到活跃接口，只统计该接口的流量
        if (activeInterface != 0) {
            for (DWORD i = 0; i < pIfTable->dwNumEntries; i++) {
                if (pIfTable->table[i].dwIndex == activeInterface) {
                    totalRecv = pIfTable->table[i].dwInOctets;
                    totalSent = pIfTable->table[i].dwOutOctets;
                    break;
                }
            }
        } else {
            // 如果没有找到活跃接口，统计所有非回环接口的流量
            for (DWORD i = 0; i < pIfTable->dwNumEntries; i++) {
                if (pIfTable->table[i].dwType != IF_TYPE_SOFTWARE_LOOPBACK) {
                    totalRecv += pIfTable->table[i].dwInOctets;
                    totalSent += pIfTable->table[i].dwOutOctets;
                }
            }
        }
        
        free(pIfTable);
        
        quint64 diffRecv = totalRecv - lastRecvBytes;
        quint64 diffSent = totalSent - lastSentBytes;
        lastRecvBytes = totalRecv;
        lastSentBytes = totalSent;
        return QPair<quint64, quint64>(diffSent, diffRecv); // 返回上传和下载速率
    }
    free(pIfTable);
    return QPair<quint64, quint64>(0, 0);
#elif defined(Q_OS_LINUX)
    std::ifstream file("/proc/net/dev");
    std::string line;
    quint64 totalRecv = 0, totalSent = 0;
    int lineNum = 0;
    while (std::getline(file, line)) {
        lineNum++;
        if (lineNum <= 2) continue; // 跳过标题行

        std::istringstream iss(line);
        std::string iface;
        quint64 recvBytes = 0, sentBytes = 0;

        iss >> iface;
        iface = iface.substr(0, iface.find(':'));
        iss >> recvBytes;
        for (int i = 0; i < 7; ++i) iss >> std::ws >> sentBytes; // Skip to transmitted bytes

        if (iface != "lo") {
            totalRecv += recvBytes;
            totalSent += sentBytes;
        }
    }
    quint64 diffRecv = totalRecv - lastRecvBytes;
    quint64 diffSent = totalSent - lastSentBytes;
    lastRecvBytes = totalRecv;
    lastSentBytes = totalSent;

    return QPair<quint64, quint64>(diffSent, diffRecv); // 返回上传和下载速率
#endif
    return QPair<quint64, quint64>(0, 0);
}

#ifdef Q_OS_WIN
DWORD NetworkMonitor::getActiveNetworkInterface() const {
    PMIB_IFTABLE pIfTable;
    DWORD dwSize = sizeof(MIB_IFTABLE);
    pIfTable = (PMIB_IFTABLE)malloc(dwSize);
    
    if (GetIfTable(pIfTable, &dwSize, TRUE) == ERROR_INSUFFICIENT_BUFFER) {
        free(pIfTable);
        pIfTable = (PMIB_IFTABLE)malloc(dwSize);
    }
    
    if (GetIfTable(pIfTable, &dwSize, TRUE) == NO_ERROR) {
        DWORD activeIndex = 0;
        quint64 maxTraffic = 0;
        
        // 获取当前时间点的数据
        for (DWORD i = 0; i < pIfTable->dwNumEntries; i++) {
            // 排除回环接口和禁用的接口
            if (pIfTable->table[i].dwType != IF_TYPE_SOFTWARE_LOOPBACK && 
                pIfTable->table[i].dwOperStatus == IF_OPER_STATUS_OPERATIONAL) {
                
                quint64 currentTraffic = pIfTable->table[i].dwInOctets + pIfTable->table[i].dwOutOctets;
                
                // 如果当前接口有流量且大于之前找到的最大流量，则更新活跃接口
                if (currentTraffic > maxTraffic) {
                    maxTraffic = currentTraffic;
                    activeIndex = pIfTable->table[i].dwIndex;
                }
            }
        }
        
        free(pIfTable);
        return activeIndex;
    }
    
    free(pIfTable);
    return 0; // 如果没有找到活跃接口，返回0
}
#else
unsigned long NetworkMonitor::getActiveNetworkInterface() const {
    return 0; // 非Windows平台暂不支持此功能
}
#endif
