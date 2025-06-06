#pragma once

#include <QObject>
#include <QVector>
#include <QPair>
#include <QDateTime>
#include <QMap>
#include <QString>
#include <QMutex>

// 自适应采样与数据压缩类 - 优化数据存储效率
class AdaptiveSampler : public QObject {
    Q_OBJECT

public:
    // 采样策略枚举
    enum SamplingStrategy {
        FixedRate,           // 固定采样率
        AdaptiveRate,        // 自适应采样率
        EventBased,          // 事件触发采样
        DeltaBased           // 变化量触发采样
    };
    
    // 压缩算法枚举
    enum CompressionAlgorithm {
        None,                // 不压缩
        RunLength,           // 游程编码
        DeltaEncoding,       // 增量编码
        Piecewise            // 分段线性压缩
    };

    explicit AdaptiveSampler(QObject *parent = nullptr);
    ~AdaptiveSampler();

    // 设置采样策略
    void setSamplingStrategy(SamplingStrategy strategy);
    
    // 获取当前采样策略
    SamplingStrategy getSamplingStrategy() const;
    
    // 设置压缩算法
    void setCompressionAlgorithm(CompressionAlgorithm algorithm);
    
    // 获取当前压缩算法
    CompressionAlgorithm getCompressionAlgorithm() const;
    
    // 设置基础采样间隔（毫秒）
    void setBaseSamplingInterval(int msec);
    
    // 获取基础采样间隔
    int getBaseSamplingInterval() const;
    
    // 设置最小采样间隔（毫秒）
    void setMinSamplingInterval(int msec);
    
    // 获取最小采样间隔
    int getMinSamplingInterval() const;
    
    // 设置最大采样间隔（毫秒）
    void setMaxSamplingInterval(int msec);
    
    // 获取最大采样间隔
    int getMaxSamplingInterval() const;
    
    // 设置变化阈值（用于DeltaBased策略）
    void setDeltaThreshold(double threshold);
    
    // 获取变化阈值
    double getDeltaThreshold() const;
    
    // 添加数据点（返回是否应该存储）
    bool addDataPoint(const QString &metricName, double value, const QDateTime &timestamp = QDateTime::currentDateTime());
    
    // 压缩历史数据
    QVector<QPair<QDateTime, double>> compressData(const QVector<QPair<QDateTime, double>> &data);
    
    // 解压数据
    QVector<QPair<QDateTime, double>> decompressData(const QVector<QPair<QDateTime, double>> &compressedData);
    
    // 获取当前采样间隔（考虑自适应策略）
    int getCurrentSamplingInterval(const QString &metricName) const;
    
    // 获取压缩率统计
    double getCompressionRatio() const;
    
    // 重置采样器状态
    void reset();

signals:
    // 采样间隔变化信号
    void samplingIntervalChanged(const QString &metricName, int newInterval);
    
    // 压缩率变化信号
    void compressionRatioChanged(double ratio);

private:
    // 根据策略决定是否应该采样
    bool shouldSample(const QString &metricName, double value, const QDateTime &timestamp);
    
    // 更新自适应采样间隔
    void updateAdaptiveInterval(const QString &metricName, double value);
    
    // 游程编码压缩
    QVector<QPair<QDateTime, double>> runLengthEncode(const QVector<QPair<QDateTime, double>> &data);
    
    // 游程编码解压
    QVector<QPair<QDateTime, double>> runLengthDecode(const QVector<QPair<QDateTime, double>> &compressedData);
    
    // 增量编码压缩
    QVector<QPair<QDateTime, double>> deltaEncode(const QVector<QPair<QDateTime, double>> &data);
    
    // 增量编码解压
    QVector<QPair<QDateTime, double>> deltaDecode(const QVector<QPair<QDateTime, double>> &compressedData);
    
    // 分段线性压缩
    QVector<QPair<QDateTime, double>> piecewiseCompress(const QVector<QPair<QDateTime, double>> &data);
    
    // 分段线性解压
    QVector<QPair<QDateTime, double>> piecewiseDecompress(const QVector<QPair<QDateTime, double>> &compressedData);

private:
    SamplingStrategy m_samplingStrategy;         // 采样策略
    CompressionAlgorithm m_compressionAlgorithm; // 压缩算法
    int m_baseSamplingInterval;                  // 基础采样间隔（毫秒）
    int m_minSamplingInterval;                   // 最小采样间隔（毫秒）
    int m_maxSamplingInterval;                   // 最大采样间隔（毫秒）
    double m_deltaThreshold;                     // 变化阈值
    
    // 每个指标的最后一个数据点
    QMap<QString, QPair<QDateTime, double>> m_lastDataPoints;
    
    // 每个指标的当前采样间隔
    QMap<QString, int> m_currentIntervals;
    
    // 每个指标的变化率历史
    QMap<QString, QVector<double>> m_changeRates;
    
    // 压缩统计
    qint64 m_originalDataSize;                   // 原始数据大小
    qint64 m_compressedDataSize;                 // 压缩后数据大小
    
    // 线程安全
    mutable QMutex m_mutex;
};