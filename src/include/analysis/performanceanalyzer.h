#pragma once

#include <QObject>
#include <QVector>
#include <QPair>
#include <QDateTime>
#include <QString>
#include <QMap>

// 性能分析类 - 提供系统性能趋势分析和瓶颈识别
class PerformanceAnalyzer : public QObject {
    Q_OBJECT

public:
    // 性能瓶颈类型枚举
    enum BottleneckType {
        None,
        CPU,
        Memory,
        Disk,
        Network,
        Multiple
    };

    // 性能趋势类型枚举
    enum TrendType {
        Stable,
        Increasing,
        Decreasing,
        Fluctuating
    };
    
    // 优化建议类型枚举
    enum SuggestionType {
        General,        // 一般性建议
        CpuRelated,     // CPU相关建议
        MemoryRelated,  // 内存相关建议
        DiskRelated,    // 磁盘相关建议
        NetworkRelated, // 网络相关建议
        ProcessRelated, // 进程相关建议
        SystemRelated   // 系统相关建议
    };
    
    // 优化建议优先级
    enum SuggestionPriority {
        Low,
        Medium,
        High,
        Critical
    };

    explicit PerformanceAnalyzer(QObject *parent = nullptr);
    ~PerformanceAnalyzer();

    // 添加性能数据点
    void addDataPoint(double cpuUsage, double memoryUsage, double diskIO, double networkUsage, 
                     const QDateTime& timestamp = QDateTime::currentDateTime());

    // 分析性能瓶颈
    BottleneckType analyzeBottleneck();

    // 获取瓶颈详情
    QString getBottleneckDetails() const;

    // 分析CPU使用趋势
    TrendType analyzeCpuTrend(int timeWindowMinutes = 30);

    // 分析内存使用趋势
    TrendType analyzeMemoryTrend(int timeWindowMinutes = 30);

    // 分析磁盘IO趋势
    TrendType analyzeDiskTrend(int timeWindowMinutes = 30);

    // 分析网络使用趋势
    TrendType analyzeNetworkTrend(int timeWindowMinutes = 30);

    // 获取趋势分析详情
    QString getTrendDetails() const;

    // 获取性能优化建议
    QString getOptimizationSuggestions() const;
    
    // 获取特定类型的优化建议
    QString getOptimizationSuggestionsByType(SuggestionType type) const;
    
    // 获取特定优先级的优化建议
    QString getOptimizationSuggestionsByPriority(SuggestionPriority priority) const;
    
    // 获取优化建议类型的字符串描述
    QString getSuggestionTypeString(SuggestionType type) const;
    
    // 获取优化建议优先级的字符串描述
    QString getSuggestionPriorityString(SuggestionPriority priority) const;

    // 更新CPU使用率
    void updateCpuUsage(double usage);
    
    // 更新内存使用率
    void updateMemoryUsage(double usage);
    
    // 更新磁盘IO使用率
    void updateDiskUsage(double usage);
    
    // 更新网络使用率
    void updateNetworkUsage(double usage);

    // 获取最新的CPU使用率
    double getLastCpuUsage() const;
    
    // 获取最新的内存使用率
    double getLastMemoryUsage() const;
    
    // 获取最新的磁盘IO
    double getLastDiskIO() const;
    
    // 获取最新的网络使用率
    double getLastNetworkUsage() const;

    // 清除历史数据
    void clearHistory();

    // 设置历史数据保留时间（小时）
    void setHistoryRetention(int hours);
    
    // 设置瓶颈阈值
    void setBottleneckThresholds(double cpuThreshold, double memoryThreshold, 
                                double diskThreshold, double networkThreshold);
    
    // 重置为默认瓶颈阈值
    void resetToDefaultThresholds();
    
    // 导出性能分析报告
    bool exportPerformanceReport(const QString& filePath) const;
    
    // 分析系统性能历史
    QMap<QString, QVariant> analyzePerformanceHistory(int days = 7) const;
    
    // 预测未来性能趋势
    QMap<QString, QVariant> predictPerformanceTrend(int hours = 24) const;

signals:
    // 检测到瓶颈时发出信号
    void bottleneckDetected(BottleneckType type, const QString& details);

    // 检测到明显趋势变化时发出信号
    void trendChanged(const QString& component, TrendType trend, const QString& details);
    
    // 性能数据更新信号
    void performanceDataUpdated(double cpuUsage, double memoryUsage, double diskIO, double networkUsage);

public:
    // 系统优化相关方法
    // 进程管理
    QList<QPair<QString, double>> getHighCpuProcesses() const;
    QList<QPair<QString, double>> getHighMemoryProcesses() const;
    bool terminateProcess(const QString &processName) const;
    
    // 线程池管理
    int getCurrentThreadPoolSize() const;
    bool setThreadPoolConfiguration(int threadCount, int priority);
    
    // 磁盘优化
    QList<QPair<QString, double>> getDiskUsage() const;
    bool optimizeDiskDefrag() const;
    bool cleanTempFiles() const;
    bool optimizeDiskCache() const;
    
    // 网络优化
    QList<QPair<QString, QString>> getNetworkInterfaces() const;
    bool optimizeDnsSettings(const QString &interfaceName) const;
    bool optimizeTcpIpSettings(const QString &interfaceName) const;
    bool configureQosPolicy(const QString &interfaceName) const;

private:
    // 计算线性回归斜率
    double calculateSlope(const QVector<QPair<QDateTime, double>>& data, int timeWindowMinutes);

    // 计算变异系数
    double calculateCoefficientOfVariation(const QVector<double>& values);
    
    // 计算标准差
    double calculateStandardDeviation(const QVector<double>& values) const;
    
    // 计算线性回归参数
    void calculateLinearRegression(const QVector<QPair<double, double>>& points, 
                                double& slope, double& intercept) const;

    // 清理过期数据
    void cleanupOldData();

    // 获取趋势类型的字符串描述
    QString trendTypeToString(TrendType trend) const;

    // 存储历史数据，格式为 <时间戳, 值>
    QVector<QPair<QDateTime, double>> m_cpuHistory;
    QVector<QPair<QDateTime, double>> m_memoryHistory;
    QVector<QPair<QDateTime, double>> m_diskHistory;
    QVector<QPair<QDateTime, double>> m_networkHistory;
    
    // 当前使用率
    double m_cpuUsage;
    double m_memoryUsage;
    double m_diskIO;
    double m_networkUsage;
    
    // 最大历史数据点数量
    int m_maxHistorySize = 1000;

    // 存储最近检测到的瓶颈和趋势
    QString m_bottleneckDetails;
    QMap<QString, QString> m_trendDetails;

    // 历史数据保留时间（小时）
    int m_retentionHours;

    // 瓶颈阈值
    double m_cpuBottleneckThreshold;
    double m_memoryBottleneckThreshold;
    double m_diskBottleneckThreshold;
    double m_networkBottleneckThreshold;
};