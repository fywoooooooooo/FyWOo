#include "src/include/analysis/nlpqueryanalyzer.h"
#include <QDebug>
#include <QRegularExpression>
#include <QDateTime>
#include <QMap>

NLPQueryAnalyzer::NLPQueryAnalyzer(QObject *parent)
    : QObject(parent)
    , m_performanceAnalyzer(nullptr)
    , m_dataStorage(nullptr)
{
    // 初始化关键词映射
    initializeKeywords();
}

NLPQueryAnalyzer::~NLPQueryAnalyzer()
{
}

void NLPQueryAnalyzer::setPerformanceAnalyzer(PerformanceAnalyzer *analyzer)
{
    m_performanceAnalyzer = analyzer;
}

void NLPQueryAnalyzer::setDataStorage(DataStorage *storage)
{
    m_dataStorage = storage;
}

QVariant NLPQueryAnalyzer::processQuery(const QString &query)
{
    qDebug() << "处理自然语言查询: " << query;
    
    // 转换为小写以便匹配
    QString lowerQuery = query.toLower();
    
    // 检查是否包含时间范围
    QDateTime startTime, endTime;
    extractTimeRange(lowerQuery, startTime, endTime);
    
    // 检查查询类型
    if (lowerQuery.contains("cpu") || lowerQuery.contains("处理器")) {
        return processCpuQuery(lowerQuery, startTime, endTime);
    } 
    else if (lowerQuery.contains("内存") || lowerQuery.contains("memory")) {
        return processMemoryQuery(lowerQuery, startTime, endTime);
    }
    else if (lowerQuery.contains("磁盘") || lowerQuery.contains("disk") || 
             lowerQuery.contains("存储") || lowerQuery.contains("io")) {
        return processDiskQuery(lowerQuery, startTime, endTime);
    }
    else if (lowerQuery.contains("网络") || lowerQuery.contains("network") || 
             lowerQuery.contains("流量") || lowerQuery.contains("带宽")) {
        return processNetworkQuery(lowerQuery, startTime, endTime);
    }
    else if (lowerQuery.contains("进程") || lowerQuery.contains("process") || 
             lowerQuery.contains("应用") || lowerQuery.contains("程序")) {
        return processProcessQuery(lowerQuery, startTime, endTime);
    }
    else if (lowerQuery.contains("系统") || lowerQuery.contains("system") || 
             lowerQuery.contains("整体") || lowerQuery.contains("全局")) {
        return processSystemQuery(lowerQuery, startTime, endTime);
    }
    
    return QVariant("无法理解您的查询，请尝试更具体的问题，例如'过去一小时内CPU使用率最高的时刻'或'当前内存占用最高的进程'。");
}

void NLPQueryAnalyzer::initializeKeywords()
{
    // 时间关键词
    m_timeKeywords["过去"] = -1;
    m_timeKeywords["最近"] = -1;
    m_timeKeywords["上一"] = -1;
    m_timeKeywords["前一"] = -1;
    m_timeKeywords["这一"] = 0;
    m_timeKeywords["当前"] = 0;
    m_timeKeywords["现在"] = 0;
    m_timeKeywords["本"] = 0;
    
    // 时间单位
    m_timeUnits["分钟"] = 60;
    m_timeUnits["小时"] = 3600;
    m_timeUnits["天"] = 86400;
    m_timeUnits["周"] = 604800;
    m_timeUnits["月"] = 2592000; // 约30天
    
    // 统计类型关键词
    m_statKeywords["最高"] = StatType::Maximum;
    m_statKeywords["最大"] = StatType::Maximum;
    m_statKeywords["峰值"] = StatType::Maximum;
    m_statKeywords["最低"] = StatType::Minimum;
    m_statKeywords["最小"] = StatType::Minimum;
    m_statKeywords["平均"] = StatType::Average;
    m_statKeywords["均值"] = StatType::Average;
    m_statKeywords["中位数"] = StatType::Median;
    m_statKeywords["趋势"] = StatType::Trend;
    m_statKeywords["变化"] = StatType::Trend;
}

void NLPQueryAnalyzer::extractTimeRange(const QString &query, QDateTime &startTime, QDateTime &endTime)
{
    // 默认为当前时间
    endTime = QDateTime::currentDateTime();
    
    // 匹配时间表达式，如"过去3小时"、"最近一天"等
    QRegularExpression timeRegex("(过去|最近|上一|前一|这一|当前|本)\\s*(\\d*)\\s*(分钟|小时|天|周|月)");
    QRegularExpressionMatch match = timeRegex.match(query);
    
    if (match.hasMatch()) {
        QString timeKeyword = match.captured(1);
        QString timeValue = match.captured(2);
        QString timeUnit = match.captured(3);
        
        int direction = m_timeKeywords.value(timeKeyword, 0);
        int unitSeconds = m_timeUnits.value(timeUnit, 3600); // 默认为小时
        int value = timeValue.isEmpty() ? 1 : timeValue.toInt();
        
        int seconds = value * unitSeconds;
        
        if (direction < 0) {
            // 过去的时间
            startTime = endTime.addSecs(-seconds);
        } else {
            // 当前时间段
            QDateTime now = QDateTime::currentDateTime();
            
            if (timeUnit == "分钟") {
                startTime = QDateTime(now.date(), QTime(now.time().hour(), now.time().minute(), 0));
            } else if (timeUnit == "小时") {
                startTime = QDateTime(now.date(), QTime(now.time().hour(), 0, 0));
            } else if (timeUnit == "天") {
                startTime = QDateTime(now.date(), QTime(0, 0, 0));
            } else if (timeUnit == "周") {
                startTime = QDateTime(now.date().addDays(-(now.date().dayOfWeek() - 1)), QTime(0, 0, 0));
            } else if (timeUnit == "月") {
                startTime = QDateTime(QDate(now.date().year(), now.date().month(), 1), QTime(0, 0, 0));
            }
            
            endTime = startTime.addSecs(seconds);
            if (endTime > now) {
                endTime = now;
            }
        }
    } else {
        // 默认查询最近一小时的数据
        startTime = endTime.addSecs(-3600);
    }
}

NLPQueryAnalyzer::StatType NLPQueryAnalyzer::extractStatType(const QString &query)
{
    for (auto it = m_statKeywords.begin(); it != m_statKeywords.end(); ++it) {
        if (query.contains(it.key())) {
            return it.value();
        }
    }
    
    // 默认返回最大值
    return StatType::Maximum;
}

QVariant NLPQueryAnalyzer::processCpuQuery(const QString &query, const QDateTime &startTime, const QDateTime &endTime)
{
    if (!m_performanceAnalyzer || !m_dataStorage) {
        return QVariant("无法访问性能分析器或数据存储");
    }
    
    StatType statType = extractStatType(query);
    QVariantMap result;
    
    // 获取指定时间范围内的CPU数据
    QVector<QPair<QDateTime, double>> cpuData = m_dataStorage->getCpuData(startTime, endTime);
    
    if (cpuData.isEmpty()) {
        return QVariant("指定时间范围内没有CPU数据");
    }
    
    switch (statType) {
        case StatType::Maximum: {
            // 查找最大值
            auto maxIt = std::max_element(cpuData.begin(), cpuData.end(), 
                [](const QPair<QDateTime, double> &a, const QPair<QDateTime, double> &b) {
                    return a.second < b.second;
                });
            
            result["type"] = "最大CPU使用率";
            result["value"] = maxIt->second;
            result["time"] = maxIt->first;
            result["unit"] = "%";
            break;
        }
        case StatType::Minimum: {
            // 查找最小值
            auto minIt = std::min_element(cpuData.begin(), cpuData.end(), 
                [](const QPair<QDateTime, double> &a, const QPair<QDateTime, double> &b) {
                    return a.second < b.second;
                });
            
            result["type"] = "最小CPU使用率";
            result["value"] = minIt->second;
            result["time"] = minIt->first;
            result["unit"] = "%";
            break;
        }
        case StatType::Average: {
            // 计算平均值
            double sum = 0.0;
            for (const auto &data : cpuData) {
                sum += data.second;
            }
            double average = sum / cpuData.size();
            
            result["type"] = "平均CPU使用率";
            result["value"] = average;
            result["startTime"] = startTime;
            result["endTime"] = endTime;
            result["unit"] = "%";
            break;
        }
        case StatType::Median: {
            // 计算中位数
            QVector<double> values;
            for (const auto &data : cpuData) {
                values.append(data.second);
            }
            std::sort(values.begin(), values.end());
            double median = values.size() % 2 == 0 ?
                (values[values.size()/2 - 1] + values[values.size()/2]) / 2.0 :
                values[values.size()/2];
            
            result["type"] = "CPU使用率中位数";
            result["value"] = median;
            result["startTime"] = startTime;
            result["endTime"] = endTime;
            result["unit"] = "%";
            break;
        }
        case StatType::Trend: {
            // 分析趋势
            PerformanceAnalyzer::TrendType trend = m_performanceAnalyzer->analyzeCpuTrend(
                static_cast<int>(startTime.secsTo(endTime) / 60));
            
            result["type"] = "CPU使用率趋势";
            result["trend"] = static_cast<int>(trend);
            result["trendName"] = trendTypeToString(trend);
            result["startTime"] = startTime;
            result["endTime"] = endTime;
            break;
        }
    }
    
    // 如果查询包含进程相关关键词，添加高CPU进程信息
    if (query.contains("进程") || query.contains("process") || 
        query.contains("应用") || query.contains("程序")) {
        QList<QPair<QString, double>> highCpuProcesses = m_performanceAnalyzer->getHighCpuProcesses();
        QVariantList processList;
        
        for (const auto &process : highCpuProcesses) {
            QVariantMap processInfo;
            processInfo["name"] = process.first;
            processInfo["usage"] = process.second;
            processList.append(processInfo);
        }
        
        result["processes"] = processList;
    }
    
    return result;
}

QVariant NLPQueryAnalyzer::processMemoryQuery(const QString &query, const QDateTime &startTime, const QDateTime &endTime)
{
    if (!m_performanceAnalyzer || !m_dataStorage) {
        return QVariant("无法访问性能分析器或数据存储");
    }
    
    StatType statType = extractStatType(query);
    QVariantMap result;
    
    // 获取指定时间范围内的内存数据
    QVector<QPair<QDateTime, double>> memoryData = m_dataStorage->getMemoryData(startTime, endTime);
    
    if (memoryData.isEmpty()) {
        return QVariant("指定时间范围内没有内存数据");
    }
    
    switch (statType) {
        case StatType::Maximum: {
            // 查找最大值
            auto maxIt = std::max_element(memoryData.begin(), memoryData.end(), 
                [](const QPair<QDateTime, double> &a, const QPair<QDateTime, double> &b) {
                    return a.second < b.second;
                });
            
            result["type"] = "最大内存使用率";
            result["value"] = maxIt->second;
            result["time"] = maxIt->first;
            result["unit"] = "%";
            break;
        }
        case StatType::Minimum: {
            // 查找最小值
            auto minIt = std::min_element(memoryData.begin(), memoryData.end(), 
                [](const QPair<QDateTime, double> &a, const QPair<QDateTime, double> &b) {
                    return a.second < b.second;
                });
            
            result["type"] = "最小内存使用率";
            result["value"] = minIt->second;
            result["time"] = minIt->first;
            result["unit"] = "%";
            break;
        }
        case StatType::Average: {
            // 计算平均值
            double sum = 0.0;
            for (const auto &data : memoryData) {
                sum += data.second;
            }
            double average = sum / memoryData.size();
            
            result["type"] = "平均内存使用率";
            result["value"] = average;
            result["startTime"] = startTime;
            result["endTime"] = endTime;
            result["unit"] = "%";
            break;
        }
        case StatType::Median: {
            // 计算中位数
            QVector<double> values;
            for (const auto &data : memoryData) {
                values.append(data.second);
            }
            std::sort(values.begin(), values.end());
            double median = values.size() % 2 == 0 ?
                (values[values.size()/2 - 1] + values[values.size()/2]) / 2.0 :
                values[values.size()/2];
            
            result["type"] = "内存使用率中位数";
            result["value"] = median;
            result["startTime"] = startTime;
            result["endTime"] = endTime;
            result["unit"] = "%";
            break;
        }
        case StatType::Trend: {
            // 分析趋势
            PerformanceAnalyzer::TrendType trend = m_performanceAnalyzer->analyzeMemoryTrend(
                static_cast<int>(startTime.secsTo(endTime) / 60));
            
            result["type"] = "内存使用率趋势";
            result["trend"] = static_cast<int>(trend);
            result["trendName"] = trendTypeToString(trend);
            result["startTime"] = startTime;
            result["endTime"] = endTime;
            break;
        }
    }
    
    // 如果查询包含进程相关关键词，添加高内存进程信息
    if (query.contains("进程") || query.contains("process") || 
        query.contains("应用") || query.contains("程序")) {
        QList<QPair<QString, double>> highMemProcesses = m_performanceAnalyzer->getHighMemoryProcesses();
        QVariantList processList;
        
        for (const auto &process : highMemProcesses) {
            QVariantMap processInfo;
            processInfo["name"] = process.first;
            processInfo["usage"] = process.second;
            processList.append(processInfo);
        }
        
        result["processes"] = processList;
    }
    
    // 如果查询包含内存泄漏关键词，添加可能的内存泄漏信息
    if (query.contains("泄漏") || query.contains("leak") || 
        query.contains("异常增长") || query.contains("异常使用")) {
        result["leakDetection"] = true;
        
        // 检查内存趋势，如果是持续增长可能存在泄漏
        if (m_performanceAnalyzer->analyzeMemoryTrend(60) == PerformanceAnalyzer::TrendType::Increasing) {
            result["possibleLeak"] = true;
            result["leakInfo"] = "检测到内存持续增长，可能存在内存泄漏";
        } else {
            result["possibleLeak"] = false;
            result["leakInfo"] = "未检测到明显的内存泄漏迹象";
        }
    }
    
    return result;
}

QVariant NLPQueryAnalyzer::processDiskQuery(const QString &query, const QDateTime &startTime, const QDateTime &endTime)
{
    if (!m_performanceAnalyzer || !m_dataStorage) {
        return QVariant("无法访问性能分析器或数据存储");
    }
    
    StatType statType = extractStatType(query);
    QVariantMap result;
    
    // 获取指定时间范围内的磁盘数据
    QVector<QPair<QDateTime, double>> diskData = m_dataStorage->getDiskData(startTime, endTime);
    
    if (diskData.isEmpty()) {
        return QVariant("指定时间范围内没有磁盘数据");
    }
    
    switch (statType) {
        case StatType::Maximum: {
            // 查找最大值
            auto maxIt = std::max_element(diskData.begin(), diskData.end(), 
                [](const QPair<QDateTime, double> &a, const QPair<QDateTime, double> &b) {
                    return a.second < b.second;
                });
            
            result["type"] = "最大磁盘IO";
            result["value"] = maxIt->second;
            result["time"] = maxIt->first;
            result["unit"] = "MB/s";
            break;
        }
        case StatType::Minimum: {
            // 查找最小值
            auto minIt = std::min_element(diskData.begin(), diskData.end(), 
                [](const QPair<QDateTime, double> &a, const QPair<QDateTime, double> &b) {
                    return a.second < b.second;
                });
            
            result["type"] = "最小磁盘IO";
            result["value"] = minIt->second;
            result["time"] = minIt->first;
            result["unit"] = "MB/s";
            break;
        }
        case StatType::Average: {
            // 计算平均值
            double sum = 0.0;
            for (const auto &data : diskData) {
                sum += data.second;
            }
            double average = sum / diskData.size();
            
            result["type"] = "平均磁盘IO";
            result["value"] = average;
            result["startTime"] = startTime;
            result["endTime"] = endTime;
            result["unit"] = "MB/s";
            break;
        }
        case StatType::Median: {
            // 计算中位数
            QVector<double> values;
            for (const auto &data : diskData) {
                values.append(data.second);
            }
            std::sort(values.begin(), values.end());
            double median = values.size() % 2 == 0 ?
                (values[values.size()/2 - 1] + values[values.size()/2]) / 2.0 :
                values[values.size()/2];
            
            result["type"] = "磁盘IO中位数";
            result["value"] = median;
            result["startTime"] = startTime;
            result["endTime"] = endTime;
            result["unit"] = "MB/s";
            break;
        }
        case StatType::Trend: {
            // 分析趋势
            PerformanceAnalyzer::TrendType trend = m_performanceAnalyzer->analyzeDiskTrend(
                static_cast<int>(startTime.secsTo(endTime) / 60));
            
            result["type"] = "磁盘IO趋势";
            result["trend"] = static_cast<int>(trend);
            result["trendName"] = trendTypeToString(trend);
            result["startTime"] = startTime;
            result["endTime"] = endTime;
            break;
        }
    }
    
    // 如果查询包含磁盘使用情况关键词，添加磁盘使用信息
    if (query.contains("使用") || query.contains("usage") || 
        query.contains("容量") || query.contains("空间")) {
        QList<QPair<QString, double>> diskUsage = m_performanceAnalyzer->getDiskUsage();
        QVariantList diskList;
        
        for (const auto &disk : diskUsage) {
            QVariantMap diskInfo;
            diskInfo["name"] = disk.first;
            diskInfo["usage"] = disk.second;
            diskList.append(diskInfo);
        }
        
        result["disks"] = diskList;
    }
    
    return result;
}

QVariant NLPQueryAnalyzer::processNetworkQuery(const QString &query, const QDateTime &startTime, const QDateTime &endTime)
{
    if (!m_performanceAnalyzer || !m_dataStorage) {
        return QVariant("无法访问性能分析器或数据存储");
    }
    
    StatType statType = extractStatType(query);
    QVariantMap result;
    
    // 获取指定时间范围内的网络数据
    QVector<QPair<QDateTime, double>> networkData = m_dataStorage->getNetworkData(startTime, endTime);
    
    if (networkData.isEmpty()) {
        return QVariant("指定时间范围内没有网络数据");
    }
    
    switch (statType) {
        case StatType::Maximum: {
            // 查找最大值
            auto maxIt = std::max_element(networkData.begin(), networkData.end(), 
                [](const QPair<QDateTime, double> &a, const QPair<QDateTime, double> &b) {
                    return a.second < b.second;
                });
            
            result["type"] = "最大网络流量";
            result["value"] = maxIt->second;
            result["time"] = maxIt->first;
            result["unit"] = "KB/s";
            break;
        }
        case StatType::Minimum: {
            // 查找最小值
            auto minIt = std::min_element(networkData.begin(), networkData.end(), 
                [](const QPair<QDateTime, double> &a, const QPair<QDateTime, double> &b) {
                    return a.second < b.second;
                });
            
            result["type"] = "最小网络流量";
            result["value"] = minIt->second;
            result["time"] = minIt->first;
            result["unit"] = "KB/s";
            break;
        }
        case StatType::Average: {
            // 计算平均值
            double sum = 0.0;
            for (const auto &data : networkData) {
                sum += data.second;
            }
            double average = sum / networkData.size();
            
            result["type"] = "平均网络流量";
            result["value"] = average;
            result["startTime"] = startTime;
            result["endTime"] = endTime;
            result["unit"] = "KB/s";
            break;
        }
        case StatType::Median: {
            // 计算中位数
            QVector<double> values;
            for (const auto &data : networkData) {
                values.append(data.second);
            }
            std::sort(values.begin(), values.end());
            double median = values.size() % 2 == 0 ?
                (values[values.size()/2 - 1] + values[values.size()/2]) / 2.0 :
                values[values.size()/2];
            
            result["type"] = "网络流量中位数";
            result["value"] = median;
            result["startTime"] = startTime;
            result["endTime"] = endTime;
            result["unit"] = "KB/s";
            break;
        }
        case StatType::Trend: {
            // 分析趋势
            PerformanceAnalyzer::TrendType trend = m_performanceAnalyzer->analyzeNetworkTrend(
                static_cast<int>(startTime.secsTo(endTime) / 60));
            
            result["type"] = "网络流量趋势";
            result["trend"] = static_cast<int>(trend);
            result["trendName"] = trendTypeToString(trend);
            result["startTime"] = startTime;
            result["endTime"] = endTime;
            break;
        }
    }
    
    // 如果查询包含网络接口关键词，添加网络接口信息
    if (query.contains("接口") || query.contains("interface") || 
        query.contains("网卡") || query.contains("适配器")) {
        QList<QPair<QString, QString>> networkInterfaces = m_performanceAnalyzer->getNetworkInterfaces();
        QVariantList interfaceList;
        
        for (const auto &interface : networkInterfaces) {
            QVariantMap interfaceInfo;
            interfaceInfo["name"] = interface.first;
            interfaceInfo["address"] = interface.second;
            interfaceList.append(interfaceInfo);
        }
        
        result["interfaces"] = interfaceList;
    }
    
    // 检查是否包含上传/下载关键词
    bool isUpload = query.contains("上传") || query.contains("upload");
    bool isDownload = query.contains("下载") || query.contains("download");
    
    if (isUpload || isDownload) {
        result["direction"] = isUpload ? "上传" : "下载";
    }
    
    return result;
}

QVariant NLPQueryAnalyzer::processProcessQuery(const QString &query, const QDateTime &startTime, const QDateTime &endTime)
{
    if (!m_performanceAnalyzer || !m_dataStorage) {
        return QVariant("无法访问性能分析器或数据存储");
    }
    
    QVariantMap result;
    result["type"] = "进程查询";
    
    // 获取高CPU使用率进程
    if (query.contains("cpu") || query.contains("处理器")) {
        QList<QPair<QString, double>> highCpuProcesses = m_performanceAnalyzer->getHighCpuProcesses();
        QVariantList processList;
        
        for (const auto &process : highCpuProcesses) {
            QVariantMap processInfo;
            processInfo["name"] = process.first;
            processInfo["cpuUsage"] = process.second;
            processList.append(processInfo);
        }
        
        result["cpuProcesses"] = processList;
        result["message"] = "已获取CPU使用率最高的进程列表";
    }
    
    // 获取高内存使用率进程
    if (query.contains("内存") || query.contains("memory")) {
        QList<QPair<QString, double>> highMemProcesses = m_performanceAnalyzer->getHighMemoryProcesses();
        QVariantList processList;
        
        for (const auto &process : highMemProcesses) {
            QVariantMap processInfo;
            processInfo["name"] = process.first;
            processInfo["memoryUsage"] = process.second;
            processList.append(processInfo);
        }
        
        result["memoryProcesses"] = processList;
        result["message"] = "已获取内存使用率最高的进程列表";
    }
    
    // 处理终止进程请求
    if (query.contains("终止") || query.contains("结束") || 
        query.contains("kill") || query.contains("terminate")) {
        // 提取进程名称
        QRegularExpression processNameRegex("[\\"\\']([\\w\\.]+)[\\"\\']|(?:终止|结束|kill|terminate)\\s+([\\w\\.]+)");
        QRegularExpressionMatch match = processNameRegex.match(query);
        
        if (match.hasMatch()) {
            QString processName = match.captured(1).isEmpty() ? match.captured(2) : match.captured(1);
            bool success = m_performanceAnalyzer->terminateProcess(processName);
            
            if (success) {
                result["terminateResult"] = true;
                result["message"] = QString("已成功终止进程: %1").arg(processName);
            } else {
                result["terminateResult"] = false;
                result["message"] = QString("无法终止进程: %1").arg(processName);
            }
        } else {
            result["terminateResult"] = false;
            result["message"] = "未能识别要终止的进程名称";
        }
    }
    
    return result;
}

QVariant NLPQueryAnalyzer::processSystemQuery(const QString &query, const QDateTime &startTime, const QDateTime &endTime)
{
    if (!m_performanceAnalyzer || !m_dataStorage) {
        return QVariant("无法访问性能分析器或数据存储");
    }
    
    QVariantMap result;
    result["type"] = "系统查询";
    
    // 检测系统瓶颈
    if (query.contains("瓶颈") || query.contains("bottleneck") || 
        query.contains("性能问题") || query.contains("性能瓶颈")) {
        PerformanceAnalyzer::BottleneckType bottleneck = m_performanceAnalyzer->analyzeBottleneck();
        QString bottleneckDetails = m_performanceAnalyzer->getBottleneckDetails();
        
        result["bottleneckType"] = static_cast<int>(bottleneck);
        result["bottleneckDetails"] = bottleneckDetails;
        
        switch (bottleneck) {
            case PerformanceAnalyzer::BottleneckType::CPU:
                result["bottleneckName"] = "CPU瓶颈";
                break;
            case PerformanceAnalyzer::BottleneckType::Memory:
                result["bottleneckName"] = "内存瓶颈";
                break;
            case PerformanceAnalyzer::BottleneckType::Disk:
                result["bottleneckName"] = "磁盘瓶颈";
                break;
            case PerformanceAnalyzer::BottleneckType::Network:
                result["bottleneckName"] = "网络瓶颈";
                break;
            case PerformanceAnalyzer::BottleneckType::Multiple:
                result["bottleneckName"] = "多重瓶颈";
                break;
            default:
                result["bottleneckName"] = "无瓶颈";
                break;
        }
    }
    
    // 获取性能优化建议
    if (query.contains("优化") || query.contains("建议") || 
        query.contains("suggestion") || query.contains("optimization")) {
        QString suggestions;
        
        if (query.contains("cpu") || query.contains("处理器")) {
            suggestions = m_performanceAnalyzer->getOptimizationSuggestionsByType(
                PerformanceAnalyzer::SuggestionType::CpuRelated);
            result["suggestionType"] = "CPU优化建议";
        } else if (query.contains("内存") || query.contains("memory")) {
            suggestions = m_performanceAnalyzer->getOptimizationSuggestionsByType(
                PerformanceAnalyzer::SuggestionType::MemoryRelated);
            result["suggestionType"] = "内存优化建议";
        } else if (query.contains("磁盘") || query.contains("disk") || 
                   query.contains("存储") || query.contains("io")) {
            suggestions = m_performanceAnalyzer->getOptimizationSuggestionsByType(
                PerformanceAnalyzer::SuggestionType::DiskRelated);
            result["suggestionType"] = "磁盘优化建议";
        } else if (query.contains("网络") || query.contains("network")) {
            suggestions = m_performanceAnalyzer->getOptimizationSuggestionsByType(
                PerformanceAnalyzer::SuggestionType::NetworkRelated);
            result["suggestionType"] = "网络优化建议";
        } else if (query.contains("进程") || query.contains("process")) {
            suggestions = m_performanceAnalyzer->getOptimizationSuggestionsByType(
                PerformanceAnalyzer::SuggestionType::ProcessRelated);
            result["suggestionType"] = "进程优化建议";
        } else {
            suggestions = m_performanceAnalyzer->getOptimizationSuggestions();
            result["suggestionType"] = "系统整体优化建议";
        }
        
        result["suggestions"] = suggestions;
    }
    
    // 获取系统性能历史分析
    if (query.contains("历史") || query.contains("history") || 
        query.contains("趋势") || query.contains("trend")) {
        int days = 7; // 默认分析最近7天
        
        // 尝试提取天数
        QRegularExpression daysRegex("(\\d+)\\s*天");
        QRegularExpressionMatch match = daysRegex.match(query);
        if (match.hasMatch()) {
            days = match.captured(1).toInt();
        }
        
        QMap<QString, QVariant> historyAnalysis = m_performanceAnalyzer->analyzePerformanceHistory(days);
        
        // 将QMap转换为QVariantMap
        for (auto it = historyAnalysis.begin(); it != historyAnalysis.end(); ++it) {
            result[it.key()] = it.value();
        }
        
        result["historyDays"] = days;
    }
    
    // 获取性能预测
    if (query.contains("预测") || query.contains("predict") || 
        query.contains("未来") || query.contains("future")) {
        int hours = 24; // 默认预测未来24小时
        
        // 尝试提取小时数
        QRegularExpression hoursRegex("(\\d+)\\s*小时");
        QRegularExpressionMatch match = hoursRegex.match(query);
        if (match.hasMatch()) {
            hours = match.captured(1).toInt();
        }
        
        QMap<QString, QVariant> prediction = m_performanceAnalyzer->predictPerformanceTrend(hours);
        
        // 将QMap转换为QVariantMap
        for (auto it = prediction.begin(); it != prediction.end(); ++it) {
            result["prediction_" + it.key()] = it.value();
        }
        
        result["predictionHours"] = hours;
    }
    
    // 如果没有匹配到特定查询类型，返回系统整体状态
    if (result.size() <= 1) {
        result["cpuUsage"] = m_performanceAnalyzer->getLastCpuUsage();
        result["memoryUsage"] = m_performanceAnalyzer->getLastMemoryUsage();
        result["diskIO"] = m_performanceAnalyzer->getLastDiskIO();
        result["networkUsage"] = m_performanceAnalyzer->getLastNetworkUsage();
        result["message"] = "当前系统整体状态";
    }
    
    return result;
}

QString NLPQueryAnalyzer::trendTypeToString(PerformanceAnalyzer::TrendType trend)
{
    switch (trend) {
        case PerformanceAnalyzer::TrendType::Stable:
            return "稳定";
        case PerformanceAnalyzer::TrendType::Increasing:
            return "上升";
        case PerformanceAnalyzer::TrendType::Decreasing:
            return "下降";
        case PerformanceAnalyzer::TrendType::Fluctuating:
            return "波动";
        default:
            return "未知";
    }
}