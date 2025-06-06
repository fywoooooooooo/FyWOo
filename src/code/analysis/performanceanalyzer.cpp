#include "src/include/analysis/performanceanalyzer.h"
#include <QDebug>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QThreadPool>
#include <QNetworkInterface>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>
#include <QStandardPaths>
#include <QStorageInfo>

#ifdef Q_OS_WIN
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#endif

PerformanceAnalyzer::PerformanceAnalyzer(QObject *parent)
    : QObject(parent)
    , m_retentionHours(24) // 默认保留24小时的数据
    , m_cpuBottleneckThreshold(85.0) // CPU使用率超过85%视为瓶颈
    , m_memoryBottleneckThreshold(90.0) // 内存使用率超过90%视为瓶颈
    , m_diskBottleneckThreshold(80.0) // 磁盘I/O使用率超过80%视为瓶颈
    , m_networkBottleneckThreshold(70.0) // 网络使用率超过70%视为瓶颈
{
}

PerformanceAnalyzer::~PerformanceAnalyzer()
{
}

void PerformanceAnalyzer::addDataPoint(double cpuUsage, double memoryUsage, double diskIO, double networkUsage, const QDateTime& timestamp)
{
    m_cpuHistory.append(qMakePair(timestamp, cpuUsage));
    m_memoryHistory.append(qMakePair(timestamp, memoryUsage));
    m_diskHistory.append(qMakePair(timestamp, diskIO));
    m_networkHistory.append(qMakePair(timestamp, networkUsage));
    
    cleanupOldData();
    
    // 每次添加数据点后分析瓶颈
    BottleneckType bottleneck = analyzeBottleneck();
    if (bottleneck != None) {
        emit bottleneckDetected(bottleneck, m_bottleneckDetails);
    }
    
    // 分析趋势变化
    TrendType cpuTrend = analyzeCpuTrend();
    TrendType memTrend = analyzeMemoryTrend();
    TrendType diskTrend = analyzeDiskTrend();
    TrendType netTrend = analyzeNetworkTrend();
    
    // 发出趋势变化信号
    if (cpuTrend != Stable) {
        emit trendChanged("CPU", cpuTrend, m_trendDetails["CPU"]);
    }
    if (memTrend != Stable) {
        emit trendChanged("Memory", memTrend, m_trendDetails["Memory"]);
    }
    if (diskTrend != Stable) {
        emit trendChanged("Disk", diskTrend, m_trendDetails["Disk"]);
    }
    if (netTrend != Stable) {
        emit trendChanged("Network", netTrend, m_trendDetails["Network"]);
    }
}

PerformanceAnalyzer::BottleneckType PerformanceAnalyzer::analyzeBottleneck()
{
    if (m_cpuHistory.isEmpty() || m_memoryHistory.isEmpty() || 
        m_diskHistory.isEmpty() || m_networkHistory.isEmpty()) {
        return None;
    }
    
    // 获取最新的性能数据
    double cpuUsage = m_cpuHistory.last().second;
    double memoryUsage = m_memoryHistory.last().second;
    double diskIO = m_diskHistory.last().second;
    double networkUsage = m_networkHistory.last().second;
    
    // 检查是否有多个瓶颈
    int bottleneckCount = 0;
    QString details;
    
    if (cpuUsage >= m_cpuBottleneckThreshold) {
        bottleneckCount++;
        details += QString("CPU使用率: %1% (阈值: %2%)\n")
                    .arg(cpuUsage, 0, 'f', 2)
                    .arg(m_cpuBottleneckThreshold, 0, 'f', 2);
    }
    
    if (memoryUsage >= m_memoryBottleneckThreshold) {
        bottleneckCount++;
        details += QString("内存使用率: %1% (阈值: %2%)\n")
                    .arg(memoryUsage, 0, 'f', 2)
                    .arg(m_memoryBottleneckThreshold, 0, 'f', 2);
    }
    
    if (diskIO >= m_diskBottleneckThreshold) {
        bottleneckCount++;
        details += QString("磁盘I/O使用率: %1 MB/s (阈值: %2 MB/s)\n")
                    .arg(diskIO, 0, 'f', 2)
                    .arg(m_diskBottleneckThreshold, 0, 'f', 2);
    }
    
    if (networkUsage >= m_networkBottleneckThreshold) {
        bottleneckCount++;
        details += QString("网络使用率: %1 MB/s (阈值: %2 MB/s)\n")
                    .arg(networkUsage, 0, 'f', 2)
                    .arg(m_networkBottleneckThreshold, 0, 'f', 2);
    }
    
    // 确定瓶颈类型
    BottleneckType result = None;
    
    if (bottleneckCount > 1) {
        result = Multiple;
        m_bottleneckDetails = "检测到多个性能瓶颈:\n" + details;
    } else if (cpuUsage >= m_cpuBottleneckThreshold) {
        result = CPU;
        m_bottleneckDetails = "检测到CPU瓶颈:\n" + details;
    } else if (memoryUsage >= m_memoryBottleneckThreshold) {
        result = Memory;
        m_bottleneckDetails = "检测到内存瓶颈:\n" + details;
    } else if (diskIO >= m_diskBottleneckThreshold) {
        result = Disk;
        m_bottleneckDetails = "检测到磁盘I/O瓶颈:\n" + details;
    } else if (networkUsage >= m_networkBottleneckThreshold) {
        result = Network;
        m_bottleneckDetails = "检测到网络瓶颈:\n" + details;
    }
    
    return result;
}

QString PerformanceAnalyzer::getBottleneckDetails() const
{
    if (m_bottleneckDetails.isEmpty()) {
        return "未检测到性能瓶颈";
    }
    return m_bottleneckDetails;
}

PerformanceAnalyzer::TrendType PerformanceAnalyzer::analyzeCpuTrend(int timeWindowMinutes)
{
    if (m_cpuHistory.size() < 10) {
        return Stable; // 数据点太少，无法进行有效分析
    }
    
    // 计算斜率
    double slope = calculateSlope(m_cpuHistory, timeWindowMinutes);
    
    // 提取最近的值用于计算变异系数
    QVector<double> recentValues;
    QDateTime cutoffTime = QDateTime::currentDateTime().addSecs(-timeWindowMinutes * 60);
    for (const auto& pair : m_cpuHistory) {
        if (pair.first >= cutoffTime) {
            recentValues.append(pair.second);
        }
    }
    
    // 计算变异系数
    double cv = calculateCoefficientOfVariation(recentValues);
    
    // 确定趋势类型
    TrendType trend;
    if (cv > 0.25) { // 如果变异系数大于0.25，认为是波动的
        trend = Fluctuating;
        m_trendDetails["CPU"] = QString("CPU使用率波动 (变异系数: %1)").arg(cv, 0, 'f', 2);
    } else if (slope > 0.05) { // 如果斜率大于0.05，认为是上升的
        trend = Increasing;
        m_trendDetails["CPU"] = QString("CPU使用率上升 (斜率: %1)").arg(slope, 0, 'f', 4);
    } else if (slope < -0.05) { // 如果斜率小于-0.05，认为是下降的
        trend = Decreasing;
        m_trendDetails["CPU"] = QString("CPU使用率下降 (斜率: %1)").arg(slope, 0, 'f', 4);
    } else { // 否则认为是稳定的
        trend = Stable;
        m_trendDetails["CPU"] = QString("CPU使用率稳定 (斜率: %1)").arg(slope, 0, 'f', 4);
    }
    
    return trend;
}

PerformanceAnalyzer::TrendType PerformanceAnalyzer::analyzeMemoryTrend(int timeWindowMinutes)
{
    if (m_memoryHistory.size() < 10) {
        return Stable; // 数据点太少，无法进行有效分析
    }
    
    // 计算斜率
    double slope = calculateSlope(m_memoryHistory, timeWindowMinutes);
    
    // 提取最近的值用于计算变异系数
    QVector<double> recentValues;
    QDateTime cutoffTime = QDateTime::currentDateTime().addSecs(-timeWindowMinutes * 60);
    for (const auto& pair : m_memoryHistory) {
        if (pair.first >= cutoffTime) {
            recentValues.append(pair.second);
        }
    }
    
    // 计算变异系数
    double cv = calculateCoefficientOfVariation(recentValues);
    
    // 确定趋势类型
    TrendType trend;
    if (cv > 0.15) { // 内存波动通常较小，所以阈值设低一些
        trend = Fluctuating;
        m_trendDetails["Memory"] = QString("内存使用率波动 (变异系数: %1)").arg(cv, 0, 'f', 2);
    } else if (slope > 0.03) {
        trend = Increasing;
        m_trendDetails["Memory"] = QString("内存使用率上升 (斜率: %1)").arg(slope, 0, 'f', 4);
    } else if (slope < -0.03) {
        trend = Decreasing;
        m_trendDetails["Memory"] = QString("内存使用率下降 (斜率: %1)").arg(slope, 0, 'f', 4);
    } else {
        trend = Stable;
        m_trendDetails["Memory"] = QString("内存使用率稳定 (斜率: %1)").arg(slope, 0, 'f', 4);
    }
    
    return trend;
}

PerformanceAnalyzer::TrendType PerformanceAnalyzer::analyzeDiskTrend(int timeWindowMinutes)
{
    if (m_diskHistory.size() < 10) {
        return Stable; // 数据点太少，无法进行有效分析
    }
    
    // 计算斜率
    double slope = calculateSlope(m_diskHistory, timeWindowMinutes);
    
    // 提取最近的值用于计算变异系数
    QVector<double> recentValues;
    QDateTime cutoffTime = QDateTime::currentDateTime().addSecs(-timeWindowMinutes * 60);
    for (const auto& pair : m_diskHistory) {
        if (pair.first >= cutoffTime) {
            recentValues.append(pair.second);
        }
    }
    
    // 计算变异系数
    double cv = calculateCoefficientOfVariation(recentValues);
    
    // 确定趋势类型
    TrendType trend;
    if (cv > 0.4) { // 磁盘I/O波动通常较大
        trend = Fluctuating;
        m_trendDetails["Disk"] = QString("磁盘I/O波动 (变异系数: %1)").arg(cv, 0, 'f', 2);
    } else if (slope > 0.1) {
        trend = Increasing;
        m_trendDetails["Disk"] = QString("磁盘I/O上升 (斜率: %1)").arg(slope, 0, 'f', 4);
    } else if (slope < -0.1) {
        trend = Decreasing;
        m_trendDetails["Disk"] = QString("磁盘I/O下降 (斜率: %1)").arg(slope, 0, 'f', 4);
    } else {
        trend = Stable;
        m_trendDetails["Disk"] = QString("磁盘I/O稳定 (斜率: %1)").arg(slope, 0, 'f', 4);
    }
    
    return trend;
}

PerformanceAnalyzer::TrendType PerformanceAnalyzer::analyzeNetworkTrend(int timeWindowMinutes)
{
    if (m_networkHistory.size() < 10) {
        return Stable; // 数据点太少，无法进行有效分析
    }
    
    // 计算斜率
    double slope = calculateSlope(m_networkHistory, timeWindowMinutes);
    
    // 提取最近的值用于计算变异系数
    QVector<double> recentValues;
    QDateTime cutoffTime = QDateTime::currentDateTime().addSecs(-timeWindowMinutes * 60);
    for (const auto& pair : m_networkHistory) {
        if (pair.first >= cutoffTime) {
            recentValues.append(pair.second);
        }
    }
    
    // 计算变异系数
    double cv = calculateCoefficientOfVariation(recentValues);
    
    // 确定趋势类型
    TrendType trend;
    if (cv > 0.5) { // 网络流量波动通常很大
        trend = Fluctuating;
        m_trendDetails["Network"] = QString("网络使用率波动 (变异系数: %1)").arg(cv, 0, 'f', 2);
    } else if (slope > 0.15) {
        trend = Increasing;
        m_trendDetails["Network"] = QString("网络使用率上升 (斜率: %1)").arg(slope, 0, 'f', 4);
    } else if (slope < -0.15) {
        trend = Decreasing;
        m_trendDetails["Network"] = QString("网络使用率下降 (斜率: %1)").arg(slope, 0, 'f', 4);
    } else {
        trend = Stable;
        m_trendDetails["Network"] = QString("网络使用率稳定 (斜率: %1)").arg(slope, 0, 'f', 4);
    }
    
    return trend;
}

QString PerformanceAnalyzer::getTrendDetails() const
{
    QString result;
    
    if (m_trendDetails.contains("CPU")) {
        result += "CPU: " + m_trendDetails["CPU"] + "\n";
    }
    
    if (m_trendDetails.contains("Memory")) {
        result += "内存: " + m_trendDetails["Memory"] + "\n";
    }
    
    if (m_trendDetails.contains("Disk")) {
        result += "磁盘: " + m_trendDetails["Disk"] + "\n";
    }
    
    if (m_trendDetails.contains("Network")) {
        result += "网络: " + m_trendDetails["Network"];
    }
    
    if (result.isEmpty()) {
        return "暂无趋势分析数据";
    }
    
    return result;
}

QString PerformanceAnalyzer::getOptimizationSuggestions() const
{
    QString result;
    
    // 检查CPU
    if (!m_cpuHistory.isEmpty() && m_cpuHistory.last().second > m_cpuBottleneckThreshold * 0.9) {
        result += "CPU优化建议:\n";
        result += "- 关闭不必要的高CPU占用应用程序\n";
        result += "- 检查是否有恶意软件或异常进程\n";
        result += "- 考虑CPU降频或节能设置\n\n";
    }
    
    // 检查内存
    if (!m_memoryHistory.isEmpty() && m_memoryHistory.last().second > m_memoryBottleneckThreshold * 0.9) {
        result += "内存优化建议:\n";
        result += "- 关闭不必要的内存占用大的应用程序\n";
        result += "- 检查是否有内存泄漏问题\n";
        result += "- 考虑增加虚拟内存或物理内存容量\n\n";
    }
    
    // 检查磁盘
    if (!m_diskHistory.isEmpty() && m_diskHistory.last().second > m_diskBottleneckThreshold * 0.9) {
        result += "磁盘优化建议:\n";
        result += "- 清理临时文件和不必要的大文件\n";
        result += "- 碎片整理（对于HDD）\n";
        result += "- 考虑使用更快的存储设备或增加缓存\n\n";
    }
    
    // 检查网络
    if (!m_networkHistory.isEmpty() && m_networkHistory.last().second > m_networkBottleneckThreshold * 0.9) {
        result += "网络优化建议:\n";
        result += "- 关闭不必要的网络连接和下载任务\n";
        result += "- 检查网络连接质量和带宽限制\n";
        result += "- 优化QoS设置或考虑升级网络带宽\n";
    }
    
    if (result.isEmpty()) {
        return "当前系统性能良好，无需特别优化";
    }
    
    return result;
}

QString PerformanceAnalyzer::getOptimizationSuggestionsByType(SuggestionType type) const
{
    QString result;
    
    switch (type) {
    case General:
            result += "一般性能优化建议:\n";
            result += "- 定期重启系统以释放资源\n";
            result += "- 关闭启动项中不必要的程序\n";
            result += "- 保持操作系统和驱动程序更新到最新版本\n";
        break;
        
    case CpuRelated:
            result += "CPU优化建议:\n";
            result += "- 关闭不必要的高CPU占用应用程序\n";
            result += "- 检查系统进程和后台服务\n";
            result += "- 检查是否有恶意软件或异常进程\n";
            result += "- 考虑CPU降频或节能设置\n";
            result += "- 更新处理器驱动程序和微码\n";
            if (!m_cpuHistory.isEmpty() && m_cpuHistory.last().second > m_cpuBottleneckThreshold * 0.9) {
                result += "- 当前CPU使用率较高，建议立即采取措施\n";
            }
        break;
        
    case MemoryRelated:
            result += "内存优化建议:\n";
            result += "- 关闭不必要的内存占用大的应用程序\n";
            result += "- 检查是否有内存泄漏问题\n";
            result += "- 调整虚拟内存设置\n";
            result += "- 清理系统缓存\n";
            result += "- 考虑增加物理内存容量\n";
            if (!m_memoryHistory.isEmpty() && m_memoryHistory.last().second > m_memoryBottleneckThreshold * 0.9) {
                result += "- 当前内存使用率较高，建议立即释放内存\n";
            }
        break;
        
    case DiskRelated:
            result += "磁盘优化建议:\n";
            result += "- 清理临时文件和不必要的大文件\n";
            result += "- 碎片整理（对于HDD）\n";
            result += "- 检查磁盘健康状态\n";
            result += "- 优化文件系统缓存\n";
            result += "- 考虑使用SSD或更快的存储设备\n";
            if (!m_diskHistory.isEmpty() && m_diskHistory.last().second > m_diskBottleneckThreshold * 0.9) {
                result += "- 当前磁盘I/O较高，建议减少磁盘操作\n";
            }
        break;
        
    case NetworkRelated:
            result += "网络优化建议:\n";
            result += "- 关闭不必要的网络连接和下载任务\n";
            result += "- 检查网络连接质量和带宽限制\n";
            result += "- 优化DNS设置\n";
            result += "- 优化TCP/IP参数\n";
            result += "- 设置QoS优先级\n";
            if (!m_networkHistory.isEmpty() && m_networkHistory.last().second > m_networkBottleneckThreshold * 0.9) {
                result += "- 当前网络使用率较高，建议限制网络密集型应用\n";
            }
        break;
        
    case ProcessRelated:
            result += "进程优化建议:\n";
            result += "- 识别并关闭异常进程\n";
            result += "- 调整进程优先级\n";
            result += "- 限制特定进程的资源使用\n";
            result += "- 优化应用程序线程池设置\n";
        break;
        
    case SystemRelated:
            result += "系统优化建议:\n";
            result += "- 优化电源设置\n";
            result += "- 检查硬件温度和散热情况\n";
            result += "- 更新系统组件\n";
            result += "- 调整系统调度策略\n";
            result += "- 优化BIOS/UEFI设置\n";
        break;
        
    default:
            result = "未知的优化建议类型";
        break;
    }
    
    return result;
}

QString PerformanceAnalyzer::getOptimizationSuggestionsByPriority(SuggestionPriority priority) const
{
    QString result;
    
    // 检查系统当前状态，确定各项优化建议的优先级
    bool cpuHigh = !m_cpuHistory.isEmpty() && m_cpuHistory.last().second > m_cpuBottleneckThreshold;
    bool memHigh = !m_memoryHistory.isEmpty() && m_memoryHistory.last().second > m_memoryBottleneckThreshold;
    bool diskHigh = !m_diskHistory.isEmpty() && m_diskHistory.last().second > m_diskBottleneckThreshold;
    bool netHigh = !m_networkHistory.isEmpty() && m_networkHistory.last().second > m_networkBottleneckThreshold;
    
    switch (priority) {
    case Critical:
            result += "关键优先级优化建议:\n";
            if (cpuHigh) {
                result += "- 立即关闭CPU占用过高的应用程序或服务\n";
                result += "- 检查系统是否存在恶意软件或病毒\n";
            }
            if (memHigh) {
                result += "- 立即关闭内存占用过高的应用程序或服务\n";
                result += "- 检查是否存在严重的内存泄漏问题\n";
            }
            if (diskHigh) {
                result += "- 停止过度的磁盘读写操作\n";
                result += "- 检查磁盘健康状态，防止数据丢失\n";
            }
            if (netHigh) {
                result += "- 立即关闭带宽占用过高的下载或上传任务\n";
                result += "- 检查是否存在可疑的网络连接或数据泄露\n";
        }
        break;
        
    case High:
            result += "高优先级优化建议:\n";
            if (cpuHigh || (!m_cpuHistory.isEmpty() && m_cpuHistory.last().second > m_cpuBottleneckThreshold * 0.8)) {
                result += "- 关闭不必要的CPU密集型应用程序\n";
                result += "- 调整进程优先级，保证关键应用性能\n";
            }
            if (memHigh || (!m_memoryHistory.isEmpty() && m_memoryHistory.last().second > m_memoryBottleneckThreshold * 0.8)) {
                result += "- 释放不必要的内存占用\n";
                result += "- 增加虚拟内存设置\n";
            }
            if (diskHigh || (!m_diskHistory.isEmpty() && m_diskHistory.last().second > m_diskBottleneckThreshold * 0.8)) {
                result += "- 暂停大型文件传输或备份操作\n";
                result += "- 清理系统临时文件\n";
            }
            if (netHigh || (!m_networkHistory.isEmpty() && m_networkHistory.last().second > m_networkBottleneckThreshold * 0.8)) {
                result += "- 限制网络密集型应用带宽\n";
                result += "- 优化网络QoS设置\n";
            }
        break;
        
    case Medium:
            result += "中等优先级优化建议:\n";
            result += "- 定期清理磁盘空间\n";
            result += "- 优化启动项设置\n";
            result += "- 调整电源和性能设置\n";
            result += "- 安装系统更新和驱动程序更新\n";
        break;
        
    case Low:
            result += "低优先级优化建议:\n";
            result += "- 碎片整理磁盘（对于HDD）\n";
            result += "- 清理注册表（谨慎操作）\n";
            result += "- 优化视觉效果设置\n";
            result += "- 调整系统缓存设置\n";
        break;
        
    default:
            result = "未知的优化建议优先级";
        break;
    }
    
    return result;
}

QString PerformanceAnalyzer::getSuggestionTypeString(SuggestionType type) const
{
    switch (type) {
    case General:
        return "一般性建议";
    case CpuRelated:
        return "CPU相关建议";
    case MemoryRelated:
        return "内存相关建议";
    case DiskRelated:
        return "磁盘相关建议";
    case NetworkRelated:
        return "网络相关建议";
    case ProcessRelated:
        return "进程相关建议";
    case SystemRelated:
        return "系统相关建议";
    default:
            return "未知建议类型";
    }
}

QString PerformanceAnalyzer::getSuggestionPriorityString(SuggestionPriority priority) const
{
    switch (priority) {
    case Low:
        return "低优先级";
    case Medium:
        return "中等优先级";
    case High:
        return "高优先级";
    case Critical:
        return "关键优先级";
    default:
        return "未知优先级";
    }
}

void PerformanceAnalyzer::clearHistory()
{
    m_cpuHistory.clear();
    m_memoryHistory.clear();
    m_diskHistory.clear();
    m_networkHistory.clear();
    m_bottleneckDetails.clear();
    m_trendDetails.clear();
}

void PerformanceAnalyzer::setHistoryRetention(int hours)
{
    if (hours > 0) {
        m_retentionHours = hours;
        cleanupOldData();
    }
}

void PerformanceAnalyzer::setBottleneckThresholds(double cpuThreshold, double memoryThreshold, 
                                                double diskThreshold, double networkThreshold)
{
    // 验证阈值的合法性（0-100%）
    if (cpuThreshold >= 0.0 && cpuThreshold <= 100.0) {
        m_cpuBottleneckThreshold = cpuThreshold;
    }
    
    if (memoryThreshold >= 0.0 && memoryThreshold <= 100.0) {
        m_memoryBottleneckThreshold = memoryThreshold;
    }
    
    if (diskThreshold >= 0.0) {
        m_diskBottleneckThreshold = diskThreshold;
    }
    
    if (networkThreshold >= 0.0) {
        m_networkBottleneckThreshold = networkThreshold;
    }
}

double PerformanceAnalyzer::calculateSlope(const QVector<QPair<QDateTime, double>>& data, int timeWindowMinutes)
{
    if (data.size() < 2) {
        return 0.0;
    }
    
    // 只考虑时间窗口内的数据点
    QDateTime cutoffTime = QDateTime::currentDateTime().addSecs(-timeWindowMinutes * 60);
    
    QVector<QPair<double, double>> points; // <x, y> 其中x是时间（秒），y是值
    QDateTime firstTime; // 用于计算相对时间
    
    bool isFirst = true;
    for (const auto& pair : data) {
        if (pair.first >= cutoffTime) {
            if (isFirst) {
                firstTime = pair.first;
                isFirst = false;
            }
            
            // 将时间转换为相对于第一个点的秒数
            double x = firstTime.secsTo(pair.first);
            double y = pair.second;
            
            points.append(qMakePair(x, y));
        }
    }
    
    if (points.size() < 2) {
        return 0.0;
    }
    
    // 计算线性回归斜率
    double sumX = 0.0, sumY = 0.0, sumXY = 0.0, sumX2 = 0.0;
    int n = points.size();
    
    for (const auto& point : points) {
        sumX += point.first;
        sumY += point.second;
        sumXY += point.first * point.second;
        sumX2 += point.first * point.first;
    }
    
    // 计算斜率 m = (n*sumXY - sumX*sumY) / (n*sumX2 - sumX*sumX)
    double denominator = n * sumX2 - sumX * sumX;
    if (std::abs(denominator) < 0.0001) { // 防止除以零
        return 0.0;
    }
    
    double slope = (n * sumXY - sumX * sumY) / denominator;
    return slope;
}

double PerformanceAnalyzer::calculateCoefficientOfVariation(const QVector<double>& values)
{
    if (values.isEmpty()) {
        return 0.0;
    }
    
    // 计算平均值
    double sum = std::accumulate(values.begin(), values.end(), 0.0);
    double mean = sum / values.size();
    
    if (std::abs(mean) < 0.0001) { // 防止除以零
        return 0.0;
    }
    
    // 计算标准差
    double sq_sum = 0.0;
    for (double value : values) {
        sq_sum += (value - mean) * (value - mean);
    }
    double stdev = std::sqrt(sq_sum / values.size());
    
    // 计算变异系数 CV = 标准差 / 平均值
    return stdev / mean;
}

void PerformanceAnalyzer::cleanupOldData()
{
    QDateTime cutoffTime = QDateTime::currentDateTime().addSecs(-m_retentionHours * 3600);
    
    // 清理CPU历史数据
    while (!m_cpuHistory.isEmpty() && m_cpuHistory.first().first < cutoffTime) {
        m_cpuHistory.removeFirst();
    }
    
    // 清理内存历史数据
    while (!m_memoryHistory.isEmpty() && m_memoryHistory.first().first < cutoffTime) {
        m_memoryHistory.removeFirst();
    }
    
    // 清理磁盘历史数据
    while (!m_diskHistory.isEmpty() && m_diskHistory.first().first < cutoffTime) {
        m_diskHistory.removeFirst();
    }
    
    // 清理网络历史数据
    while (!m_networkHistory.isEmpty() && m_networkHistory.first().first < cutoffTime) {
        m_networkHistory.removeFirst();
    }
}

QString PerformanceAnalyzer::trendTypeToString(TrendType trend) const
{
    switch (trend) {
    case Stable:
        return "稳定";
    case Increasing:
        return "上升";
    case Decreasing:
        return "下降";
    case Fluctuating:
        return "波动";
    default:
        return "未知";
    }
}

double PerformanceAnalyzer::getLastCpuUsage() const
{
    if (m_cpuHistory.isEmpty()) {
        return 0.0;
    }
    return m_cpuHistory.last().second;
}

double PerformanceAnalyzer::getLastMemoryUsage() const
{
    if (m_memoryHistory.isEmpty()) {
        return 0.0;
    }
    return m_memoryHistory.last().second;
}

double PerformanceAnalyzer::getLastDiskIO() const
{
    if (m_diskHistory.isEmpty()) {
        return 0.0;
    }
    return m_diskHistory.last().second;
}

double PerformanceAnalyzer::getLastNetworkUsage() const
{
    if (m_networkHistory.isEmpty()) {
        return 0.0;
    }
    return m_networkHistory.last().second;
}

bool PerformanceAnalyzer::exportPerformanceReport(const QString& filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "无法打开文件进行写入:" << filePath;
        return false;
    }
    
    QTextStream out(&file);
    
    // 添加报告标题
    out << "系统性能报告\n";
    out << "=============\n";
    out << "生成时间: " << QDateTime::currentDateTime().toString() << "\n\n";
    
    // 添加当前性能概览
    out << "当前性能概览\n";
    out << "=============\n";
    
    if (!m_cpuHistory.isEmpty()) {
        out << "CPU使用率: " << m_cpuHistory.last().second << "%\n";
    }
    
    if (!m_memoryHistory.isEmpty()) {
        out << "内存使用率: " << m_memoryHistory.last().second << "%\n";
    }
    
    if (!m_diskHistory.isEmpty()) {
        out << "磁盘I/O: " << m_diskHistory.last().second << " MB/s\n";
    }
    
    if (!m_networkHistory.isEmpty()) {
        out << "网络使用率: " << m_networkHistory.last().second << " MB/s\n";
    }
    
    out << "\n";
    
    // 添加瓶颈分析
    out << "瓶颈分析\n";
    out << "=============\n";
    out << const_cast<PerformanceAnalyzer*>(this)->getBottleneckDetails() << "\n\n";
    
    // 添加趋势分析
    out << "趋势分析\n";
    out << "=============\n";
    out << getTrendDetails() << "\n\n";
    
    // 添加优化建议
    out << "优化建议\n";
    out << "=============\n";
    out << getOptimizationSuggestions() << "\n";
    
    // 添加历史数据统计
    out << "历史数据统计\n";
    out << "=============\n";
    
    if (!m_cpuHistory.isEmpty()) {
        QVector<double> cpuValues;
        for (const auto& pair : m_cpuHistory) {
            cpuValues.append(pair.second);
        }
        
        double cpuAvg = std::accumulate(cpuValues.begin(), cpuValues.end(), 0.0) / cpuValues.size();
        double cpuMax = *std::max_element(cpuValues.begin(), cpuValues.end());
        double cpuMin = *std::min_element(cpuValues.begin(), cpuValues.end());
        
        out << "CPU使用率统计:\n";
        out << "  - 平均值: " << cpuAvg << "%\n";
        out << "  - 最大值: " << cpuMax << "%\n";
        out << "  - 最小值: " << cpuMin << "%\n";
        out << "  - 数据点数: " << m_cpuHistory.size() << "\n\n";
    }
    
    if (!m_memoryHistory.isEmpty()) {
        QVector<double> memValues;
        for (const auto& pair : m_memoryHistory) {
            memValues.append(pair.second);
        }
        
        double memAvg = std::accumulate(memValues.begin(), memValues.end(), 0.0) / memValues.size();
        double memMax = *std::max_element(memValues.begin(), memValues.end());
        double memMin = *std::min_element(memValues.begin(), memValues.end());
        
        out << "内存使用率统计:\n";
        out << "  - 平均值: " << memAvg << "%\n";
        out << "  - 最大值: " << memMax << "%\n";
        out << "  - 最小值: " << memMin << "%\n";
        out << "  - 数据点数: " << m_memoryHistory.size() << "\n";
    }
    
    file.close();
    return true;
}

QMap<QString, QVariant> PerformanceAnalyzer::analyzePerformanceHistory(int days) const
{
    QMap<QString, QVariant> result;
    
    // 设置时间窗口
    QDateTime cutoffTime = QDateTime::currentDateTime().addDays(-days);
    
    // 提取时间窗口内的数据
    QVector<QPair<QDateTime, double>> filteredCpuHistory;
    QVector<QPair<QDateTime, double>> filteredMemoryHistory;
    QVector<QPair<QDateTime, double>> filteredDiskHistory;
    QVector<QPair<QDateTime, double>> filteredNetworkHistory;
    
    for (const auto& pair : m_cpuHistory) {
        if (pair.first >= cutoffTime) {
            filteredCpuHistory.append(pair);
        }
    }
    
    for (const auto& pair : m_memoryHistory) {
        if (pair.first >= cutoffTime) {
            filteredMemoryHistory.append(pair);
        }
    }
    
    for (const auto& pair : m_diskHistory) {
        if (pair.first >= cutoffTime) {
            filteredDiskHistory.append(pair);
        }
    }
    
    for (const auto& pair : m_networkHistory) {
        if (pair.first >= cutoffTime) {
            filteredNetworkHistory.append(pair);
        }
    }
    
    // 分析CPU历史数据
    if (!filteredCpuHistory.isEmpty()) {
        QVector<double> cpuValues;
        for (const auto& pair : filteredCpuHistory) {
            cpuValues.append(pair.second);
        }
        
        double cpuAvg = std::accumulate(cpuValues.begin(), cpuValues.end(), 0.0) / cpuValues.size();
        double cpuMax = *std::max_element(cpuValues.begin(), cpuValues.end());
        double cpuMin = *std::min_element(cpuValues.begin(), cpuValues.end());
        double cpuStdDev = calculateStandardDeviation(cpuValues);
        
        QMap<QString, QVariant> cpuStats;
        cpuStats["average"] = cpuAvg;
        cpuStats["max"] = cpuMax;
        cpuStats["min"] = cpuMin;
        cpuStats["stddev"] = cpuStdDev;
        cpuStats["dataPoints"] = cpuValues.size();
        
        result["cpu"] = cpuStats;
    }
    
    // 分析内存历史数据
    if (!filteredMemoryHistory.isEmpty()) {
        QVector<double> memValues;
        for (const auto& pair : filteredMemoryHistory) {
            memValues.append(pair.second);
        }
        
        double memAvg = std::accumulate(memValues.begin(), memValues.end(), 0.0) / memValues.size();
        double memMax = *std::max_element(memValues.begin(), memValues.end());
        double memMin = *std::min_element(memValues.begin(), memValues.end());
        double memStdDev = calculateStandardDeviation(memValues);
        
        QMap<QString, QVariant> memStats;
        memStats["average"] = memAvg;
        memStats["max"] = memMax;
        memStats["min"] = memMin;
        memStats["stddev"] = memStdDev;
        memStats["dataPoints"] = memValues.size();
        
        result["memory"] = memStats;
    }
    
    // 分析磁盘历史数据
    if (!filteredDiskHistory.isEmpty()) {
        QVector<double> diskValues;
        for (const auto& pair : filteredDiskHistory) {
            diskValues.append(pair.second);
        }
        
        double diskAvg = std::accumulate(diskValues.begin(), diskValues.end(), 0.0) / diskValues.size();
        double diskMax = *std::max_element(diskValues.begin(), diskValues.end());
        double diskMin = *std::min_element(diskValues.begin(), diskValues.end());
        double diskStdDev = calculateStandardDeviation(diskValues);
        
        QMap<QString, QVariant> diskStats;
        diskStats["average"] = diskAvg;
        diskStats["max"] = diskMax;
        diskStats["min"] = diskMin;
        diskStats["stddev"] = diskStdDev;
        diskStats["dataPoints"] = diskValues.size();
        
        result["disk"] = diskStats;
    }
    
    // 分析网络历史数据
    if (!filteredNetworkHistory.isEmpty()) {
        QVector<double> netValues;
        for (const auto& pair : filteredNetworkHistory) {
            netValues.append(pair.second);
        }
        
        double netAvg = std::accumulate(netValues.begin(), netValues.end(), 0.0) / netValues.size();
        double netMax = *std::max_element(netValues.begin(), netValues.end());
        double netMin = *std::min_element(netValues.begin(), netValues.end());
        double netStdDev = calculateStandardDeviation(netValues);
        
        QMap<QString, QVariant> netStats;
        netStats["average"] = netAvg;
        netStats["max"] = netMax;
        netStats["min"] = netMin;
        netStats["stddev"] = netStdDev;
        netStats["dataPoints"] = netValues.size();
        
        result["network"] = netStats;
    }
    
    return result;
}

QMap<QString, QVariant> PerformanceAnalyzer::predictPerformanceTrend(int hours) const
{
    QMap<QString, QVariant> result;
    
    // 获取当前时间作为参考点
    QDateTime now = QDateTime::currentDateTime();
    
    // 将历史数据转换为数据点
    QVector<QPair<double, double>> cpuPoints;
    QVector<QPair<double, double>> memPoints;
    QVector<QPair<double, double>> diskPoints;
    QVector<QPair<double, double>> netPoints;
    
    int i = 0;
    for (const auto& pair : m_cpuHistory) {
        // 将时间转换为小时数（相对于当前时间）
        double hoursDiff = pair.first.secsTo(now) / 3600.0;
        cpuPoints.append(qMakePair(-hoursDiff, pair.second));
        i++;
    }
    
    i = 0;
    for (const auto& pair : m_memoryHistory) {
        double hoursDiff = pair.first.secsTo(now) / 3600.0;
        memPoints.append(qMakePair(-hoursDiff, pair.second));
        i++;
    }
    
    i = 0;
    for (const auto& pair : m_diskHistory) {
        double hoursDiff = pair.first.secsTo(now) / 3600.0;
        diskPoints.append(qMakePair(-hoursDiff, pair.second));
        i++;
    }
    
    i = 0;
    for (const auto& pair : m_networkHistory) {
        double hoursDiff = pair.first.secsTo(now) / 3600.0;
        netPoints.append(qMakePair(-hoursDiff, pair.second));
        i++;
    }
    
    // 计算线性回归参数
    double cpuSlope = 0.0, cpuIntercept = 0.0;
    double memSlope = 0.0, memIntercept = 0.0;
    double diskSlope = 0.0, diskIntercept = 0.0;
    double netSlope = 0.0, netIntercept = 0.0;
    
    if (!cpuPoints.isEmpty()) {
        calculateLinearRegression(cpuPoints, cpuSlope, cpuIntercept);
        double cpuPrediction = cpuIntercept + cpuSlope * hours;
        cpuPrediction = qBound(0.0, cpuPrediction, 100.0);
        result["CPU使用率预测"] = QString::number(cpuPrediction, 'f', 2) + "%";
        
        if (cpuPrediction > m_cpuBottleneckThreshold) {
            result["CPU瓶颈可能性"] = "高";
        } else if (cpuPrediction > m_cpuBottleneckThreshold * 0.8) {
            result["CPU瓶颈可能性"] = "中等";
        } else {
            result["CPU瓶颈可能性"] = "低";
        }
    }
    
    if (!memPoints.isEmpty()) {
    calculateLinearRegression(memPoints, memSlope, memIntercept);
        double memPrediction = memIntercept + memSlope * hours;
        memPrediction = qBound(0.0, memPrediction, 100.0);
        result["内存使用率预测"] = QString::number(memPrediction, 'f', 2) + "%";
        
        if (memPrediction > m_memoryBottleneckThreshold) {
            result["内存瓶颈可能性"] = "高";
        } else if (memPrediction > m_memoryBottleneckThreshold * 0.8) {
            result["内存瓶颈可能性"] = "中等";
        } else {
            result["内存瓶颈可能性"] = "低";
        }
    }
    
    return result;
}

// 计算标准差的辅助方法
double PerformanceAnalyzer::calculateStandardDeviation(const QVector<double>& values) const
{
    if (values.isEmpty()) {
        return 0.0;
    }
    
    double sum = std::accumulate(values.begin(), values.end(), 0.0);
    double mean = sum / values.size();
    
    double sq_sum = 0.0;
    for (double value : values) {
        sq_sum += (value - mean) * (value - mean);
    }
    
    return std::sqrt(sq_sum / values.size());
}

// 计算线性回归参数的辅助方法
void PerformanceAnalyzer::calculateLinearRegression(const QVector<QPair<double, double>>& points, 
                                                 double& slope, double& intercept) const
{
    if (points.size() < 2) {
        slope = 0.0;
        intercept = 0.0;
        return;
    }
    
    double sumX = 0.0, sumY = 0.0, sumXY = 0.0, sumX2 = 0.0;
    int n = points.size();
    
    for (const auto& point : points) {
        sumX += point.first;
        sumY += point.second;
        sumXY += point.first * point.second;
        sumX2 += point.first * point.first;
    }
    
    // 计算斜率和截距
    double denominator = n * sumX2 - sumX * sumX;
    if (std::abs(denominator) < 0.0001) { // 防止除以零
        slope = 0.0;
        intercept = sumY / n;
    } else {
        slope = (n * sumXY - sumX * sumY) / denominator;
        intercept = (sumY - slope * sumX) / n;
    }
}

// 系统优化相关方法实现
// 进程管理
QList<QPair<QString, double>> PerformanceAnalyzer::getHighCpuProcesses() const
{
    QList<QPair<QString, double>> result;
    
#ifdef Q_OS_WIN
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return result;
    }
    
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    if (!Process32First(hSnapshot, &pe32)) {
        CloseHandle(hSnapshot);
        return result;
    }
    
    // 收集进程信息
    QMap<DWORD, QPair<QString, FILETIME>> processes;
    do {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
        if (hProcess) {
            FILETIME ftCreation, ftExit, ftKernel, ftUser;
            if (GetProcessTimes(hProcess, &ftCreation, &ftExit, &ftKernel, &ftUser)) {
                processes[pe32.th32ProcessID] = qMakePair(
                    QString::fromWCharArray(pe32.szExeFile),
                    ftUser // 只关注用户模式CPU时间
                );
            }
            CloseHandle(hProcess);
        }
    } while (Process32Next(hSnapshot, &pe32));
    
    CloseHandle(hSnapshot);
    
    // 等待一段时间再次获取CPU时间
    QThread::msleep(500);
    
    // 再次查询CPU时间
    QList<QPair<QString, double>> cpuUsages;
    for (auto it = processes.begin(); it != processes.end(); ++it) {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, it.key());
        if (hProcess) {
            FILETIME ftCreation, ftExit, ftKernel, ftUser;
            if (GetProcessTimes(hProcess, &ftCreation, &ftExit, &ftKernel, &ftUser)) {
                // 计算用户模式CPU时间差
                ULARGE_INTEGER ul1, ul2;
                ul1.LowPart = it.value().second.dwLowDateTime;
                ul1.HighPart = it.value().second.dwHighDateTime;
                ul2.LowPart = ftUser.dwLowDateTime;
                ul2.HighPart = ftUser.dwHighDateTime;
                double cpuUsage = (ul2.QuadPart - ul1.QuadPart) / 5000.0; // 除以时间间隔（转换为百分比）
                
                cpuUsages.append(qMakePair(it.value().first, cpuUsage));
            }
            CloseHandle(hProcess);
        }
    }
    
    // 按CPU使用率排序
    std::sort(cpuUsages.begin(), cpuUsages.end(), 
             [](const QPair<QString, double>& a, const QPair<QString, double>& b) {
                 return a.second > b.second;
             });
    
    // 返回最高的前10个
    for (int i = 0; i < qMin(10, cpuUsages.size()); ++i) {
        result.append(cpuUsages[i]);
    }
#else
    // Linux实现
    QProcess process;
    process.start("ps", QStringList() << "-eo" << "pid,pcpu,comm" << "--sort=-pcpu" << "--no-headers");
    if (process.waitForFinished()) {
        QString output = process.readAllStandardOutput();
        QTextStream stream(&output);
        int count = 0;
        while (!stream.atEnd() && count < 10) {
            QString line = stream.readLine().trimmed();
            QStringList parts = line.split(QRegExp("\\s+"), QString::SkipEmptyParts);
            if (parts.size() >= 3) {
                QString processName = parts[2];
                double cpuUsage = parts[1].toDouble();
                result.append(qMakePair(processName, cpuUsage));
                count++;
            }
        }
    }
#endif
    
    return result;
}

bool PerformanceAnalyzer::cleanTempFiles() const
{
    bool success = true;
    
    // 清理系统临时文件夹
    QDir tempDir(QDir::tempPath());
    QStringList entries = tempDir.entryList(QDir::NoDotAndDotDot | QDir::Files);
    
    foreach (const QString &file, entries) {
        if (!tempDir.remove(file)) {
            qWarning() << "无法删除临时文件: " << tempDir.filePath(file);
            success = false;
        }
    }
    
    // 清理应用程序缓存目录
    QDir cacheDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    if (cacheDir.exists()) {
        entries = cacheDir.entryList(QDir::NoDotAndDotDot | QDir::Files);
        foreach (const QString &file, entries) {
            if (!cacheDir.remove(file)) {
                qWarning() << "无法删除缓存文件: " << cacheDir.filePath(file);
                success = false;
            }
        }
    }
    
    return success;
}

bool PerformanceAnalyzer::optimizeDiskCache() const
{
#ifdef Q_OS_WIN
    // 在Windows上调整磁盘缓存设置
    QProcess process;
    process.start("powershell", QStringList() << "-Command" << "Get-WmiObject Win32_Volume | ForEach-Object {$_.DriveLetter + ' ' + $_.IndexingEnabled}");
    process.waitForFinished();
    
    QString output = process.readAllStandardOutput();
    qDebug() << "当前磁盘索引状态: " << output;
    
    // 这里只是示例，实际应用中可能需要更复杂的逻辑
    return true;
#else
    // 在Linux/macOS上可能使用不同的命令
    qWarning() << "磁盘缓存优化功能在当前操作系统上不可用";
    return false;
#endif
}

QList<QPair<QString, QString>> PerformanceAnalyzer::getNetworkInterfaces() const
{
    QList<QPair<QString, QString>> result;
    
    // 获取所有网络接口
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    
    foreach (const QNetworkInterface &interface, interfaces) {
        if (interface.flags().testFlag(QNetworkInterface::IsUp) && 
            !interface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
            
            QString name = interface.humanReadableName();
            QString info;
            
            // 获取IP地址信息
            QList<QNetworkAddressEntry> entries = interface.addressEntries();
            foreach (const QNetworkAddressEntry &entry, entries) {
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                    info += "IP: " + entry.ip().toString() + ", ";
                    info += "掩码: " + entry.netmask().toString();
                    break;
                }
            }
            
            result.append(qMakePair(name, info));
        }
    }
    
    return result;
}

bool PerformanceAnalyzer::optimizeDnsSettings(const QString &interface) const
{
#ifdef Q_OS_WIN
    // 在Windows上查看当前DNS设置
    QProcess process;
    process.start("ipconfig", QStringList() << "/all");
    process.waitForFinished();
    
    QString output = process.readAllStandardOutput();
    qDebug() << "当前网络配置: " << output;
    
    // 这里只是示例，实际应用中可能需要更复杂的逻辑
    // 例如，可以设置使用公共DNS服务器如8.8.8.8
    qDebug() << "尝试优化接口的DNS设置: " << interface;
    return true;
#else
    // 在Linux/macOS上可能使用不同的命令
    qWarning() << "DNS设置优化功能在当前操作系统上不可用";
    return false;
#endif
}

bool PerformanceAnalyzer::optimizeTcpIpSettings(const QString &interface) const
{
#ifdef Q_OS_WIN
    // 在Windows上查看当前TCP/IP设置
    QProcess process;
    process.start("netsh", QStringList() << "interface" << "ipv4" << "show" << "subinterfaces");
    process.waitForFinished();
    
    QString output = process.readAllStandardOutput();
    qDebug() << "当前TCP/IP配置: " << output;
    
    // 这里只是示例，实际应用中可能需要更复杂的逻辑
    qDebug() << "尝试优化接口的TCP/IP设置: " << interface;
    return true;
#else
    // 在Linux/macOS上可能使用不同的命令
    qWarning() << "TCP/IP设置优化功能在当前操作系统上不可用";
    return false;
#endif
}

bool PerformanceAnalyzer::configureQosPolicy(const QString &interface) const
{
#ifdef Q_OS_WIN
    // 在Windows上配置QoS策略
    QProcess process;
    process.start("netsh", QStringList() << "qos" << "show" << "policy");
    process.waitForFinished();
    
    QString output = process.readAllStandardOutput();
    qDebug() << "当前QoS策略: " << output;
    
    // 这里只是示例，实际应用中可能需要更复杂的逻辑
    qDebug() << "尝试为接口配置QoS策略: " << interface;
    return true;
#else
    // 在Linux/macOS上可能使用不同的命令
    qWarning() << "QoS策略配置功能在当前操作系统上不可用";
    return false;
#endif
}

// 系统优化相关方法实现
// 进程管理
QList<QPair<QString, double>> PerformanceAnalyzer::getHighMemoryProcesses() const
{
    QList<QPair<QString, double>> result;
    
#ifdef Q_OS_WIN
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return result;
    }
    
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    if (!Process32First(hSnapshot, &pe32)) {
        CloseHandle(hSnapshot);
        return result;
    }
    
    QList<QPair<QString, double>> memUsages;
    do {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
        if (hProcess) {
            PROCESS_MEMORY_COUNTERS pmc;
            if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
                // 获取内存使用量（MB）
                double memUsage = static_cast<double>(pmc.WorkingSetSize) / (1024 * 1024);
                memUsages.append(qMakePair(QString::fromWCharArray(pe32.szExeFile), memUsage));
            }
            CloseHandle(hProcess);
        }
    } while (Process32Next(hSnapshot, &pe32));
    
    CloseHandle(hSnapshot);
    
    // 按内存使用率排序
    std::sort(memUsages.begin(), memUsages.end(), 
             [](const QPair<QString, double>& a, const QPair<QString, double>& b) {
                 return a.second > b.second;
             });
    
    // 返回最高的前10个
    for (int i = 0; i < qMin(10, memUsages.size()); ++i) {
        result.append(memUsages[i]);
    }
#else
    // Linux实现
    QProcess process;
    process.start("ps", QStringList() << "-eo" << "pid,pmem,comm" << "--sort=-pmem" << "--no-headers");
    if (process.waitForFinished()) {
        QString output = process.readAllStandardOutput();
        QTextStream stream(&output);
        int count = 0;
        while (!stream.atEnd() && count < 10) {
            QString line = stream.readLine().trimmed();
            QStringList parts = line.split(QRegExp("\\s+"), QString::SkipEmptyParts);
            if (parts.size() >= 3) {
                QString processName = parts[2];
                double memUsage = parts[1].toDouble();
                result.append(qMakePair(processName, memUsage));
                count++;
            }
        }
    }
#endif
    
    return result;
}

bool PerformanceAnalyzer::terminateProcess(const QString &processName) const
{
#ifdef Q_OS_WIN
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        qWarning() << "Failed to create process snapshot";
        return false;
    }
    
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    if (!Process32First(hSnapshot, &pe32)) {
        qWarning() << "Failed to get first process";
        CloseHandle(hSnapshot);
        return false;
    }
    
    bool found = false;
    do {
        QString currentProcessName = QString::fromWCharArray(pe32.szExeFile);
        if (currentProcessName.compare(processName, Qt::CaseInsensitive) == 0) {
            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
            if (hProcess != NULL) {
                if (TerminateProcess(hProcess, 0)) {
                    found = true;
                }
                CloseHandle(hProcess);
            }
        }
    } while (!found && Process32Next(hSnapshot, &pe32));
    
    CloseHandle(hSnapshot);
    return found;
#else
    Q_UNUSED(processName);
    return false;
#endif
}

void PerformanceAnalyzer::resetToDefaultThresholds()
{
    m_cpuBottleneckThreshold = 85.0;
    m_memoryBottleneckThreshold = 90.0;
    m_diskBottleneckThreshold = 80.0;
    m_networkBottleneckThreshold = 70.0;
}

void PerformanceAnalyzer::updateCpuUsage(double usage) {
    // 限制CPU使用率不超过100%
    double normalizedUsage = qMin(100.0, usage);
    
    m_cpuUsage = normalizedUsage;
    
    // 使用当前时间戳添加数据点
    QDateTime now = QDateTime::currentDateTime();
    m_cpuHistory.append(qMakePair(now, normalizedUsage));
    
    // 保持历史数据合理长度
    while (m_cpuHistory.size() > m_maxHistorySize) {
        m_cpuHistory.removeFirst();
    }
    
    // 检查是否为CPU瓶颈
    BottleneckType bottleneck = analyzeBottleneck();
    if (bottleneck != None) {
        emit bottleneckDetected(bottleneck, m_bottleneckDetails);
    }
    
    // 分析趋势变化
    TrendType cpuTrend = analyzeCpuTrend();
    if (cpuTrend != Stable) {
        emit trendChanged("CPU", cpuTrend, m_trendDetails["CPU"]);
    }
    
    // 性能数据更新信号
    emit performanceDataUpdated(normalizedUsage, m_memoryUsage, m_diskIO, m_networkUsage);
}

void PerformanceAnalyzer::updateMemoryUsage(double usage) {
    // 限制内存使用率不超过100%
    double normalizedUsage = qMin(100.0, usage);
    
    m_memoryUsage = normalizedUsage;
    
    // 使用当前时间戳添加数据点
    QDateTime now = QDateTime::currentDateTime();
    m_memoryHistory.append(qMakePair(now, normalizedUsage));
    
    // 保持历史数据合理长度
    while (m_memoryHistory.size() > m_maxHistorySize) {
        m_memoryHistory.removeFirst();
    }
    
    // 检查是否为内存瓶颈
    BottleneckType bottleneck = analyzeBottleneck();
    if (bottleneck != None) {
        emit bottleneckDetected(bottleneck, m_bottleneckDetails);
    }
    
    // 分析趋势变化
    TrendType memoryTrend = analyzeMemoryTrend();
    if (memoryTrend != Stable) {
        emit trendChanged("Memory", memoryTrend, m_trendDetails["Memory"]);
    }
    
    // 性能数据更新信号
    emit performanceDataUpdated(m_cpuUsage, normalizedUsage, m_diskIO, m_networkUsage);
}

void PerformanceAnalyzer::updateDiskUsage(double usage) {
    // 磁盘IO可能超过100MB/s，所以不需要限制上限
    m_diskIO = usage;
    
    // 使用当前时间戳添加数据点
    QDateTime now = QDateTime::currentDateTime();
    m_diskHistory.append(qMakePair(now, usage));
    
    // 保持历史数据合理长度
    while (m_diskHistory.size() > m_maxHistorySize) {
        m_diskHistory.removeFirst();
    }
    
    // 检查是否为磁盘瓶颈
    BottleneckType bottleneck = analyzeBottleneck();
    if (bottleneck != None) {
        emit bottleneckDetected(bottleneck, m_bottleneckDetails);
    }
    
    // 分析趋势变化
    TrendType diskTrend = analyzeDiskTrend();
    if (diskTrend != Stable) {
        emit trendChanged("Disk", diskTrend, m_trendDetails["Disk"]);
    }
    
    // 性能数据更新信号
    emit performanceDataUpdated(m_cpuUsage, m_memoryUsage, usage, m_networkUsage);
}

void PerformanceAnalyzer::updateNetworkUsage(double usage) {
    // 网络带宽可能超过100MB/s，所以不需要限制上限
    m_networkUsage = usage;
    
    // 使用当前时间戳添加数据点
    QDateTime now = QDateTime::currentDateTime();
    m_networkHistory.append(qMakePair(now, usage));
    
    // 保持历史数据合理长度
    while (m_networkHistory.size() > m_maxHistorySize) {
        m_networkHistory.removeFirst();
    }
    
    // 检查是否为网络瓶颈
    BottleneckType bottleneck = analyzeBottleneck();
    if (bottleneck != None) {
        emit bottleneckDetected(bottleneck, m_bottleneckDetails);
    }
    
    // 分析趋势变化
    TrendType networkTrend = analyzeNetworkTrend();
    if (networkTrend != Stable) {
        emit trendChanged("Network", networkTrend, m_trendDetails["Network"]);
    }
    
    // 性能数据更新信号
    emit performanceDataUpdated(m_cpuUsage, m_memoryUsage, m_diskIO, usage);
}

int PerformanceAnalyzer::getCurrentThreadPoolSize() const
{
    // 获取全局线程池大小
    return QThreadPool::globalInstance()->maxThreadCount();
}

bool PerformanceAnalyzer::setThreadPoolConfiguration(int threadCount, int priority)
{
    try {
        // 设置全局线程池线程数
        QThreadPool::globalInstance()->setMaxThreadCount(threadCount);
        
        // 设置线程优先级
        // 注意：QThreadPool不直接支持设置优先级，这里我们记录设置但实际不改变系统线程优先级
        QThread::Priority threadPriority = static_cast<QThread::Priority>(priority);
        qDebug() << "线程池配置已更新：线程数=" << threadCount << "，优先级=" << threadPriority;
        
        return true;
    } catch (const std::exception& e) {
        qWarning() << "设置线程池配置失败: " << e.what();
        return false;
    }
}

bool PerformanceAnalyzer::optimizeDiskDefrag() const
{
#ifdef Q_OS_WIN
    // 在Windows上，使用系统磁盘碎片整理工具
    qDebug() << "开始磁盘碎片整理...";
    
    // 获取系统盘符
    QStorageInfo rootStorage = QStorageInfo::root();
    QString rootPath = rootStorage.rootPath();
    QString driveLetter = rootPath.left(2); // 通常为"C:"或类似的盘符
    
    // 启动磁盘碎片整理进程
    QProcess process;
    process.start("defrag", QStringList() << driveLetter << "/A");
    
    // 这里实际环境中可能需要更长的超时时间，这里只是示例
    if (!process.waitForStarted(3000)) {
        qWarning() << "启动磁盘碎片整理失败：" << process.errorString();
    return false;
    }
    
    // 实际应用中不应该等待完成，而是应该后台运行，这里只是为了示例
    if (!process.waitForFinished(5000)) {
        qDebug() << "磁盘碎片整理已启动，但尚未完成（正在后台运行）";
        // 即使超时也认为是成功的，因为碎片整理可能需要很长时间
        return true;
    }
    
    if (process.exitCode() == 0) {
        qDebug() << "磁盘碎片整理成功";
        return true;
    } else {
        qWarning() << "磁盘碎片整理出错，错误码：" << process.exitCode();
        return false;
    }
#else
    // Linux/macOS系统
    QProcess process;
#ifdef Q_OS_LINUX
    process.start("sudo", QStringList() << "e4defrag" << "/");
#elif defined(Q_OS_MACOS)
    // macOS不需要碎片整理
    qDebug() << "macOS文件系统(APFS)不需要手动碎片整理";
    return true;
#endif
    
    if (!process.waitForStarted(3000)) {
        qWarning() << "启动磁盘碎片整理失败：" << process.errorString();
        return false;
    }

    // 实际不应等待完成
    if (!process.waitForFinished(5000)) {
        qDebug() << "磁盘碎片整理已启动，但尚未完成（正在后台运行）";
        return true;
    }

    return (process.exitCode() == 0);
#endif
}

QList<QPair<QString, double>> PerformanceAnalyzer::getDiskUsage() const
{
    QList<QPair<QString, double>> result;
    
    // 获取所有可用存储设备
    QList<QStorageInfo> storages = QStorageInfo::mountedVolumes();
    
    for (const QStorageInfo& storage : storages) {
        if (storage.isValid() && storage.isReady()) {
            QString rootPath = storage.rootPath();
            
            // 计算已用空间百分比
            qint64 total = storage.bytesTotal();
            qint64 free = storage.bytesFree();
            
            if (total > 0) {
                double usedPercentage = ((double)(total - free) / total) * 100.0;
                
                // 为显示目的格式化根路径
#ifdef Q_OS_WIN
                // Windows下显示盘符
                if (rootPath.length() >= 2 && rootPath[1] == ':') {
                    rootPath = rootPath.left(3); // 例如 "C:\"
                }
#endif
                
                result.append(qMakePair(rootPath, usedPercentage));
            }
        }
    }
    
    return result;
}