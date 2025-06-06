// processmonitor.cpp
#include "src/include/monitor/processmonitor.h"
#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#pragma comment(lib, "psapi.lib")
#elif defined(Q_OS_LINUX)
#include <dirent.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <string>
#endif

ProcessMonitor::ProcessMonitor(QObject *parent) : QObject(parent) {}

QList<ProcessInfo> ProcessMonitor::getTopProcesses(int maxCount) {
    QList<ProcessInfo> list;
#ifdef Q_OS_WIN
    DWORD pids[1024], needed;
    if (!EnumProcesses(pids, sizeof(pids), &needed)) return list;
    int count = needed / sizeof(DWORD);

    for (int i = 0; i < count; ++i) {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pids[i]);
        if (!hProcess) continue;

        TCHAR name[MAX_PATH] = TEXT("<unknown>");
        HMODULE mod;
        DWORD cbNeeded;
        if (EnumProcessModules(hProcess, &mod, sizeof(mod), &cbNeeded)) {
            GetModuleBaseName(hProcess, mod, name, sizeof(name) / sizeof(TCHAR));
        }

        PROCESS_MEMORY_COUNTERS pmc;
        double mem = 0;
        if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
            mem = pmc.WorkingSetSize / (1024.0 * 1024.0);
        }

        ProcessInfo info;
        info.name = QString::fromWCharArray(name);
        info.pid = static_cast<quint64>(pids[i]);
        info.cpuPercent = 0.0;
        info.memoryMB = mem;
        info.usage = mem; // 设置通用usage字段为内存使用量
        info.usageString = QString::number(mem, 'f', 1) + " MB";
        list.append(info);
        CloseHandle(hProcess);
    }
#elif defined(Q_OS_LINUX)
    DIR *dir = opendir("/proc");
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type != DT_DIR) continue;
        QString pidStr = entry->d_name;
        bool ok;
        int pid = pidStr.toInt(&ok);
        if (!ok) continue;

        std::ifstream statFile(QString("/proc/%1/stat").arg(pid).toStdString());
        std::ifstream statusFile(QString("/proc/%1/status").arg(pid).toStdString());
        std::string comm;
        double mem = 0;
        if (statFile) {
            std::string temp;
            statFile >> pid >> comm; // name is comm (2nd field)
            // 移除comm字符串两端的括号
            if (comm.size() >= 2 && comm.front() == '(' && comm.back() == ')') {
                comm = comm.substr(1, comm.size() - 2);
            }
        }
        if (statusFile) {
            std::string line;
            while (std::getline(statusFile, line)) {
                if (line.find("VmRSS:") == 0) {
                    double vmRss = 0;
                    sscanf(line.c_str(), "VmRSS: %lf kB", &vmRss);
                    mem = vmRss / 1024.0; // 转换为MB
                    break;
                }
            }
        }

        ProcessInfo info;
        info.name = QString::fromStdString(comm);
        info.pid = static_cast<quint64>(pid);
        info.cpuPercent = 0.0;
        info.memoryMB = mem;
        info.usage = mem; // 设置通用usage字段为内存使用量
        info.usageString = QString::number(mem, 'f', 1) + " MB";
        list.append(info);
    }
    closedir(dir);
#endif
    std::sort(list.begin(), list.end(), [](const ProcessInfo &a, const ProcessInfo &b) {
        return a.memoryMB > b.memoryMB;
    });
    if (list.size() > maxCount)
        list = list.mid(0, maxCount);
    return list;
}
