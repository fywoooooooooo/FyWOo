#include "src/include/analysis/anomalydetector.h"
#include <QDebug>
#include <cmath>
#include <numeric>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

AnomalyDetector::AnomalyDetector(QObject *parent)
    : QObject(parent)
    , m_retentionHours(24)
{
}

AnomalyDetector::~AnomalyDetector()
{
}

void AnomalyDetector::addCpuDataPoint(double value, const QDateTime& timestamp)
{
    m_cpuHistory.append(qMakePair(timestamp, value));
    cleanupOldData();
}

void AnomalyDetector::addMemoryDataPoint(double value, const QDateTime& timestamp)
{
    m_memoryHistory.append(qMakePair(timestamp, value));
    cleanupOldData();
}

void AnomalyDetector::addDiskDataPoint(double value, const QDateTime& timestamp)
{
    m_diskHistory.append(qMakePair(timestamp, value));
    cleanupOldData();
}

void AnomalyDetector::addNetworkDataPoint(double value, const QDateTime& timestamp)
{
    m_networkHistory.append(qMakePair(timestamp, value));
    cleanupOldData();
}

bool AnomalyDetector::detectCpuAnomaly(double threshold)
{
    if (m_cpuHistory.size() < 10) {
        return false; // 数据点太少，无法进行有效分析
    }
    
    // 提取最近的值
    double latestValue = m_cpuHistory.last().second;
    
    // 提取历史数据值
    QVector<double> historicalValues;
    for (const auto& pair : m_cpuHistory) {
        historicalValues.append(pair.second);
    }
    
    // 计算Z分数
    double zScore = calculateZScore(historicalValues, latestValue);
    
    // 如果Z分数超过阈值，则认为是异常
    bool isAnomaly = std::abs(zScore) > threshold;
    
    if (isAnomaly) {
        QString details = QString("CPU使用率异常: 当前值 %1%, Z分数 %2 (阈值: %3)")
                            .arg(latestValue, 0, 'f', 2)
                            .arg(zScore, 0, 'f', 2)
                            .arg(threshold, 0, 'f', 2);
        m_anomalyDetails["CPU"] = details;
        emit anomalyDetected("CPU", latestValue, threshold, m_cpuHistory.last().first);
    }
    
    return isAnomaly;
}

bool AnomalyDetector::detectMemoryAnomaly(double threshold)
{
    if (m_memoryHistory.size() < 10) {
        return false; // 数据点太少，无法进行有效分析
    }
    
    // 提取最近的值
    double latestValue = m_memoryHistory.last().second;
    
    // 提取历史数据值
    QVector<double> historicalValues;
    for (const auto& pair : m_memoryHistory) {
        historicalValues.append(pair.second);
    }
    
    // 计算Z分数
    double zScore = calculateZScore(historicalValues, latestValue);
    
    // 如果Z分数超过阈值，则认为是异常
    bool isAnomaly = std::abs(zScore) > threshold;
    
    if (isAnomaly) {
        QString details = QString("内存使用率异常: 当前值 %1%, Z分数 %2 (阈值: %3)")
                            .arg(latestValue, 0, 'f', 2)
                            .arg(zScore, 0, 'f', 2)
                            .arg(threshold, 0, 'f', 2);
        m_anomalyDetails["Memory"] = details;
        emit anomalyDetected("Memory", latestValue, threshold, m_memoryHistory.last().first);
    }
    
    return isAnomaly;
}

bool AnomalyDetector::detectDiskAnomaly(double threshold)
{
    if (m_diskHistory.size() < 10) {
        return false; // 数据点太少，无法进行有效分析
    }
    
    // 提取最近的值
    double latestValue = m_diskHistory.last().second;
    
    // 提取历史数据值
    QVector<double> historicalValues;
    for (const auto& pair : m_diskHistory) {
        historicalValues.append(pair.second);
    }
    
    // 计算Z分数
    double zScore = calculateZScore(historicalValues, latestValue);
    
    // 如果Z分数超过阈值，则认为是异常
    bool isAnomaly = std::abs(zScore) > threshold;
    
    if (isAnomaly) {
        QString details = QString("磁盘I/O异常: 当前值 %1 MB/s, Z分数 %2 (阈值: %3)")
                            .arg(latestValue, 0, 'f', 2)
                            .arg(zScore, 0, 'f', 2)
                            .arg(threshold, 0, 'f', 2);
        m_anomalyDetails["Disk"] = details;
        emit anomalyDetected("Disk", latestValue, threshold, m_diskHistory.last().first);
    }
    
    return isAnomaly;
}

bool AnomalyDetector::detectNetworkAnomaly(double threshold)
{
    if (m_networkHistory.size() < 10) {
        return false; // 数据点太少，无法进行有效分析
    }
    
    // 提取最近的值
    double latestValue = m_networkHistory.last().second;
    
    // 提取历史数据值
    QVector<double> historicalValues;
    for (const auto& pair : m_networkHistory) {
        historicalValues.append(pair.second);
    }
    
    // 计算Z分数
    double zScore = calculateZScore(historicalValues, latestValue);
    
    // 如果Z分数超过阈值，则认为是异常
    bool isAnomaly = std::abs(zScore) > threshold;
    
    if (isAnomaly) {
        QString details = QString("网络使用率异常: 当前值 %1 MB/s, Z分数 %2 (阈值: %3)")
                            .arg(latestValue, 0, 'f', 2)
                            .arg(zScore, 0, 'f', 2)
                            .arg(threshold, 0, 'f', 2);
        m_anomalyDetails["Network"] = details;
        emit anomalyDetected("Network", latestValue, threshold, m_networkHistory.last().first);
    }
    
    return isAnomaly;
}

QString AnomalyDetector::getAnomalyDetails() const
{
    if (m_anomalyDetails.isEmpty()) {
        return "未检测到异常";
    }
    
    QString result;
    for (auto it = m_anomalyDetails.constBegin(); it != m_anomalyDetails.constEnd(); ++it) {
        result += it.value() + "\n";
    }
    
    return result;
}

void AnomalyDetector::clearHistory()
{
    m_cpuHistory.clear();
    m_memoryHistory.clear();
    m_diskHistory.clear();
    m_networkHistory.clear();
    m_anomalyDetails.clear();
}

void AnomalyDetector::setHistoryRetention(int hours)
{
    if (hours > 0) {
        m_retentionHours = hours;
        cleanupOldData();
    }
}

void AnomalyDetector::setThresholdFactor(double factor)
{
    // 设置异常检测的阈值因子
    if (factor > 0) {
        m_thresholdFactor = factor;
    }
}

double AnomalyDetector::getThresholdFactor() const
{
    return m_thresholdFactor;
}

double AnomalyDetector::calculateZScore(const QVector<double>& data, double value)
{
    if (data.isEmpty()) {
        return 0.0;
    }
    
    // 计算平均值
    double sum = std::accumulate(data.begin(), data.end(), 0.0);
    double mean = sum / data.size();
    
    // 计算标准差
    double sq_sum = std::inner_product(data.begin(), data.end(), data.begin(), 0.0);
    double stdev = std::sqrt(sq_sum / data.size() - mean * mean);
    
    // 防止除以零
    if (stdev < 0.0001) {
        stdev = 0.0001;
    }
    
    // 计算Z分数
    return (value - mean) / stdev;
}

// 获取异常类型的字符串描述
QString AnomalyDetector::getAnomalyTypeString(AnomalyType type) const
{
    switch (type) {
        case NoAnomaly:
            return tr("无异常");
        case Spike:
            return tr("突发峰值");
        case Dip:
            return tr("突发低谷");
        case SustainedHigh:
            return tr("持续高值");
        case SustainedLow:
            return tr("持续低值");
        case Oscillation:
            return tr("异常波动");
        case Trend:
            return tr("异常趋势");
        default:
            return tr("未知异常");
    }
}

// 获取严重程度的字符串描述
QString AnomalyDetector::getSeverityLevelString(SeverityLevel level) const
{
    switch (level) {
        case Low:
            return tr("低");
        case Medium:
            return tr("中");
        case High:
            return tr("高");
        case Critical:
            return tr("严重");
        default:
            return tr("未知");
    }
}

// 设置异常检测算法
void AnomalyDetector::setDetectionAlgorithm(const QString& algorithm)
{
    m_detectionAlgorithm = algorithm;
    qDebug() << "异常检测算法已设置为:" << algorithm;
}

// 获取当前异常检测算法
QString AnomalyDetector::getDetectionAlgorithm() const
{
    return m_detectionAlgorithm;
}

// 设置自动学习模式
void AnomalyDetector::setAutoLearningMode(bool enabled)
{
    m_autoLearningEnabled = enabled;
    qDebug() << "自动学习模式:" << (enabled ? "已启用" : "已禁用");
}

// 获取自动学习模式状态
bool AnomalyDetector::isAutoLearningEnabled() const
{
    return m_autoLearningEnabled;
}

// 设置季节性模式检测
void AnomalyDetector::setSeasonalityDetection(bool enabled)
{
    m_seasonalityDetectionEnabled = enabled;
    qDebug() << "季节性模式检测:" << (enabled ? "已启用" : "已禁用");
}

// 获取季节性模式检测状态
bool AnomalyDetector::isSeasonalityDetectionEnabled() const
{
    return m_seasonalityDetectionEnabled;
}

// 分析历史异常模式
QMap<QString, int> AnomalyDetector::analyzeAnomalyPatterns()
{
    QMap<QString, int> patterns;
    
    // 分析CPU异常模式
    int cpuSpikes = 0;
    int cpuDips = 0;
    int cpuSustainedHigh = 0;
    
    for (int i = 1; i < m_cpuHistory.size(); ++i) {
        double current = m_cpuHistory[i].second;
        double previous = m_cpuHistory[i-1].second;
        
        // 检测峰值
        if (current > previous * 1.5 && current > 70.0) {
            cpuSpikes++;
        }
        // 检测低谷
        else if (current < previous * 0.5 && previous > 30.0) {
            cpuDips++;
        }
        // 检测持续高值
        else if (current > 85.0 && previous > 85.0) {
            cpuSustainedHigh++;
        }
    }
    
    patterns["CPU_Spikes"] = cpuSpikes;
    patterns["CPU_Dips"] = cpuDips;
    patterns["CPU_SustainedHigh"] = cpuSustainedHigh;
    
    // 类似地分析内存、磁盘和网络异常模式
    // 内存异常模式
    int memorySpikes = 0;
    int memorySustainedHigh = 0;
    
    for (int i = 1; i < m_memoryHistory.size(); ++i) {
        double current = m_memoryHistory[i].second;
        double previous = m_memoryHistory[i-1].second;
        
        if (current > previous * 1.3 && current > 80.0) {
            memorySpikes++;
        }
        else if (current > 90.0 && previous > 90.0) {
            memorySustainedHigh++;
        }
    }
    
    patterns["Memory_Spikes"] = memorySpikes;
    patterns["Memory_SustainedHigh"] = memorySustainedHigh;
    
    // 磁盘异常模式
    int diskSpikes = 0;
    int diskOscillations = 0;
    
    for (int i = 2; i < m_diskHistory.size(); ++i) {
        double current = m_diskHistory[i].second;
        double previous = m_diskHistory[i-1].second;
        double beforePrevious = m_diskHistory[i-2].second;
        
        if (current > previous * 2.0) {
            diskSpikes++;
        }
        else if ((current > previous && previous < beforePrevious) || 
                 (current < previous && previous > beforePrevious)) {
            diskOscillations++;
        }
    }
    
    patterns["Disk_Spikes"] = diskSpikes;
    patterns["Disk_Oscillations"] = diskOscillations;
    
    // 网络异常模式
    int networkSpikes = 0;
    int networkDips = 0;
    
    for (int i = 1; i < m_networkHistory.size(); ++i) {
        double current = m_networkHistory[i].second;
        double previous = m_networkHistory[i-1].second;
        
        if (current > previous * 3.0) {
            networkSpikes++;
        }
        else if (current < previous * 0.3 && previous > 10.0) {
            networkDips++;
        }
    }
    
    patterns["Network_Spikes"] = networkSpikes;
    patterns["Network_Dips"] = networkDips;
    
    return patterns;
}

// 导出异常检测报告
bool AnomalyDetector::exportAnomalyReport(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "无法打开文件进行写入:" << filePath;
        return false;
    }
    
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    
    // 写入报告标题
    out << "===== 系统资源异常检测报告 =====\n";
    out << "生成时间: " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "\n\n";
    
    // 写入当前设置
    out << "--- 当前设置 ---\n";
    out << "检测算法: " << m_detectionAlgorithm << "\n";
    out << "阈值因子: " << m_thresholdFactor << "\n";
    out << "自动学习模式: " << (m_autoLearningEnabled ? "启用" : "禁用") << "\n";
    out << "季节性检测: " << (m_seasonalityDetectionEnabled ? "启用" : "禁用") << "\n\n";
    
    // 写入异常详情
    out << "--- 检测到的异常 ---\n";
    for (auto it = m_anomalyDetails.begin(); it != m_anomalyDetails.end(); ++it) {
        out << it.key() << ": " << it.value() << "\n";
    }
    out << "\n";
    
    // 写入异常模式分析
    out << "--- 异常模式分析 ---\n";
    QMap<QString, int> patterns = analyzeAnomalyPatterns();
    for (auto it = patterns.begin(); it != patterns.end(); ++it) {
        out << it.key() << ": " << it.value() << "\n";
    }
    
    file.close();
    qDebug() << "异常检测报告已导出到:" << filePath;
    return true;
}

// 设置自定义阈值
void AnomalyDetector::setCustomThresholds(double cpuThreshold, double memoryThreshold, 
                                        double diskThreshold, double networkThreshold)
{
    m_cpuThreshold = cpuThreshold;
    m_memoryThreshold = memoryThreshold;
    m_diskThreshold = diskThreshold;
    m_networkThreshold = networkThreshold;
    
    qDebug() << "已设置自定义阈值 - CPU:" << cpuThreshold 
             << "内存:" << memoryThreshold 
             << "磁盘:" << diskThreshold 
             << "网络:" << networkThreshold;
}

// 重置为默认阈值
void AnomalyDetector::resetToDefaultThresholds()
{
    m_cpuThreshold = 80.0;
    m_memoryThreshold = 85.0;
    m_diskThreshold = 75.0;
    m_networkThreshold = 70.0;
    m_thresholdFactor = 2.0;
    
    qDebug() << "已重置为默认阈值";
}

void AnomalyDetector::cleanupOldData()
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
