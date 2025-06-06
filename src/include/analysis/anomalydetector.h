#pragma once

#include <QObject>
#include <QVector>
#include <QPair>
#include <QDateTime>
#include <QMap>

// 异常检测类 - 基于历史数据分析系统性能异常
class AnomalyDetector : public QObject {
    Q_OBJECT

public:
    // 异常类型枚举
    enum AnomalyType {
        NoAnomaly,
        Spike,          // 突发峰值
        Dip,            // 突发低谷
        SustainedHigh,  // 持续高值
        SustainedLow,   // 持续低值
        Oscillation,    // 异常波动
        Trend           // 异常趋势
    };
    
    // 异常严重程度
    enum SeverityLevel {
        Low,
        Medium,
        High,
        Critical
    };

    explicit AnomalyDetector(QObject *parent = nullptr);
    ~AnomalyDetector();

    // 添加CPU使用率数据点
    void addCpuDataPoint(double value, const QDateTime& timestamp = QDateTime::currentDateTime());
    
    // 添加内存使用率数据点
    void addMemoryDataPoint(double value, const QDateTime& timestamp = QDateTime::currentDateTime());
    
    // 添加磁盘IO数据点
    void addDiskDataPoint(double value, const QDateTime& timestamp = QDateTime::currentDateTime());
    
    // 添加网络使用率数据点
    void addNetworkDataPoint(double value, const QDateTime& timestamp = QDateTime::currentDateTime());
    
    // 获取当前CPU使用率
    double getCurrentCpuUsage() const;
    
    // 获取当前内存使用率
    double getCurrentMemoryUsage() const;
    
    // 获取当前磁盘IO
    double getCurrentDiskIO() const;
    
    // 获取当前网络使用率
    double getCurrentNetworkUsage() const;
    
    // 检测CPU异常
    bool detectCpuAnomaly(double threshold = 2.0);
    
    // 检测内存异常
    bool detectMemoryAnomaly(double threshold = 2.0);
    
    // 检测磁盘IO异常
    bool detectDiskAnomaly(double threshold = 2.0);
    
    // 检测网络异常
    bool detectNetworkAnomaly(double threshold = 2.0);
    
    // 获取异常详情
    QString getAnomalyDetails() const;
    
    // 获取异常类型的字符串描述
    QString getAnomalyTypeString(AnomalyType type) const;
    
    // 获取严重程度的字符串描述
    QString getSeverityLevelString(SeverityLevel level) const;
    
    // 清除历史数据
    void clearHistory();
    
    // 设置历史数据保留时间（小时）
    void setHistoryRetention(int hours);
    
    // 设置阈值因子
    void setThresholdFactor(double factor);
    
    // 获取当前阈值因子
    double getThresholdFactor() const;
    
    // 设置异常检测算法
    void setDetectionAlgorithm(const QString& algorithm);
    
    // 获取当前异常检测算法
    QString getDetectionAlgorithm() const;
    
    // 设置自动学习模式
    void setAutoLearningMode(bool enabled);
    
    // 获取自动学习模式状态
    bool isAutoLearningEnabled() const;
    
    // 设置季节性模式检测
    void setSeasonalityDetection(bool enabled);
    
    // 获取季节性模式检测状态
    bool isSeasonalityDetectionEnabled() const;
    
    // 分析历史异常模式
    QMap<QString, int> analyzeAnomalyPatterns();
    
    // 导出异常检测报告
    bool exportAnomalyReport(const QString& filePath);
    
    // 设置自定义阈值
    void setCustomThresholds(double cpuThreshold, double memoryThreshold, 
                            double diskThreshold, double networkThreshold);
    
    // 重置为默认阈值
    void resetToDefaultThresholds();

signals:
    // 检测到异常时发出信号
    void anomalyDetected(const QString& source, double value, double threshold, const QDateTime& timestamp);

private:
    // 计算Z分数（标准分数）
    double calculateZScore(const QVector<double>& data, double value);
    
    // 清理过期数据
    void cleanupOldData();
    
    // 存储历史数据，格式为 <时间戳, 值>
    QVector<QPair<QDateTime, double>> m_cpuHistory;
    QVector<QPair<QDateTime, double>> m_memoryHistory;
    QVector<QPair<QDateTime, double>> m_diskHistory;
    QVector<QPair<QDateTime, double>> m_networkHistory;
    
    // 存储最近检测到的异常
    QMap<QString, QString> m_anomalyDetails;
    
    // 历史数据保留时间（小时）
    int m_retentionHours;
    
    // 异常检测阈值因子
    double m_thresholdFactor = 2.0;
    
    // 异常检测算法
    QString m_detectionAlgorithm;
    
    // 自动学习模式
    bool m_autoLearningEnabled;
    
    // 季节性检测
    bool m_seasonalityDetectionEnabled;
    
    // 自定义阈值
    double m_cpuThreshold;
    double m_memoryThreshold;
    double m_diskThreshold;
    double m_networkThreshold;
};