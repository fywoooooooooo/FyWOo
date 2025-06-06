#include "src/include/storage/adaptivesampler.h"
#include <QDebug>
#include <cmath>
#include <algorithm>

AdaptiveSampler::AdaptiveSampler(QObject *parent)
    : QObject(parent)
    , m_samplingStrategy(FixedRate)
    , m_compressionAlgorithm(None)
    , m_baseSamplingInterval(1000)  // 默认1秒
    , m_minSamplingInterval(100)    // 最小100毫秒
    , m_maxSamplingInterval(60000)  // 最大1分钟
    , m_deltaThreshold(0.05)        // 默认5%变化阈值
    , m_originalDataSize(0)
    , m_compressedDataSize(0)
{
}

AdaptiveSampler::~AdaptiveSampler()
{
}

void AdaptiveSampler::setSamplingStrategy(SamplingStrategy strategy)
{
    QMutexLocker locker(&m_mutex);
    m_samplingStrategy = strategy;
    qDebug() << "设置采样策略为:" << strategy;
}

AdaptiveSampler::SamplingStrategy AdaptiveSampler::getSamplingStrategy() const
{
    QMutexLocker locker(&m_mutex);
    return m_samplingStrategy;
}

void AdaptiveSampler::setCompressionAlgorithm(CompressionAlgorithm algorithm)
{
    QMutexLocker locker(&m_mutex);
    m_compressionAlgorithm = algorithm;
    qDebug() << "设置压缩算法为:" << algorithm;
}

AdaptiveSampler::CompressionAlgorithm AdaptiveSampler::getCompressionAlgorithm() const
{
    QMutexLocker locker(&m_mutex);
    return m_compressionAlgorithm;
}

void AdaptiveSampler::setBaseSamplingInterval(int msec)
{
    QMutexLocker locker(&m_mutex);
    if (msec >= m_minSamplingInterval && msec <= m_maxSamplingInterval) {
        m_baseSamplingInterval = msec;
        qDebug() << "设置基础采样间隔为:" << msec << "毫秒";
    }
}

int AdaptiveSampler::getBaseSamplingInterval() const
{
    QMutexLocker locker(&m_mutex);
    return m_baseSamplingInterval;
}

void AdaptiveSampler::setMinSamplingInterval(int msec)
{
    QMutexLocker locker(&m_mutex);
    if (msec > 0 && msec <= m_baseSamplingInterval) {
        m_minSamplingInterval = msec;
        qDebug() << "设置最小采样间隔为:" << msec << "毫秒";
    }
}

int AdaptiveSampler::getMinSamplingInterval() const
{
    QMutexLocker locker(&m_mutex);
    return m_minSamplingInterval;
}

void AdaptiveSampler::setMaxSamplingInterval(int msec)
{
    QMutexLocker locker(&m_mutex);
    if (msec >= m_baseSamplingInterval) {
        m_maxSamplingInterval = msec;
        qDebug() << "设置最大采样间隔为:" << msec << "毫秒";
    }
}

int AdaptiveSampler::getMaxSamplingInterval() const
{
    QMutexLocker locker(&m_mutex);
    return m_maxSamplingInterval;
}

void AdaptiveSampler::setDeltaThreshold(double threshold)
{
    QMutexLocker locker(&m_mutex);
    if (threshold > 0.0 && threshold < 1.0) {
        m_deltaThreshold = threshold;
        qDebug() << "设置变化阈值为:" << threshold;
    }
}

double AdaptiveSampler::getDeltaThreshold() const
{
    QMutexLocker locker(&m_mutex);
    return m_deltaThreshold;
}

bool AdaptiveSampler::addDataPoint(const QString &metricName, double value, const QDateTime &timestamp)
{
    QMutexLocker locker(&m_mutex);
    
    // 判断是否应该采样
    bool shouldStore = shouldSample(metricName, value, timestamp);
    
    // 如果应该采样，更新最后一个数据点
    if (shouldStore) {
        m_lastDataPoints[metricName] = qMakePair(timestamp, value);
        
        // 如果是自适应策略，更新采样间隔
        if (m_samplingStrategy == AdaptiveRate) {
            updateAdaptiveInterval(metricName, value);
        }
    }
    
    return shouldStore;
}

QVector<QPair<QDateTime, double>> AdaptiveSampler::compressData(const QVector<QPair<QDateTime, double>> &data)
{
    QMutexLocker locker(&m_mutex);
    
    // 更新原始数据大小统计
    m_originalDataSize += data.size() * (sizeof(QDateTime) + sizeof(double));
    
    QVector<QPair<QDateTime, double>> compressedData;
    
    // 根据选择的压缩算法进行压缩
    switch (m_compressionAlgorithm) {
        case RunLength:
            compressedData = runLengthEncode(data);
            break;
        case DeltaEncoding:
            compressedData = deltaEncode(data);
            break;
        case Piecewise:
            compressedData = piecewiseCompress(data);
            break;
        case None:
        default:
            compressedData = data; // 不压缩
            break;
    }
    
    // 更新压缩后数据大小统计
    m_compressedDataSize += compressedData.size() * (sizeof(QDateTime) + sizeof(double));
    
    // 发出压缩率变化信号
    if (m_originalDataSize > 0) {
        double ratio = static_cast<double>(m_compressedDataSize) / m_originalDataSize;
        emit compressionRatioChanged(ratio);
    }
    
    return compressedData;
}

QVector<QPair<QDateTime, double>> AdaptiveSampler::decompressData(const QVector<QPair<QDateTime, double>> &compressedData)
{
    QMutexLocker locker(&m_mutex);
    
    QVector<QPair<QDateTime, double>> decompressedData;
    
    // 根据选择的压缩算法进行解压
    switch (m_compressionAlgorithm) {
        case RunLength:
            decompressedData = runLengthDecode(compressedData);
            break;
        case DeltaEncoding:
            decompressedData = deltaDecode(compressedData);
            break;
        case Piecewise:
            decompressedData = piecewiseDecompress(compressedData);
            break;
        case None:
        default:
            decompressedData = compressedData; // 不解压
            break;
    }
    
    return decompressedData;
}

int AdaptiveSampler::getCurrentSamplingInterval(const QString &metricName) const
{
    QMutexLocker locker(&m_mutex);
    
    // 如果不是自适应策略，返回基础采样间隔
    if (m_samplingStrategy != AdaptiveRate) {
        return m_baseSamplingInterval;
    }
    
    // 如果没有为该指标设置间隔，返回基础采样间隔
    if (!m_currentIntervals.contains(metricName)) {
        return m_baseSamplingInterval;
    }
    
    return m_currentIntervals[metricName];
}

double AdaptiveSampler::getCompressionRatio() const
{
    QMutexLocker locker(&m_mutex);
    
    if (m_originalDataSize == 0) {
        return 1.0; // 没有数据，压缩率为1
    }
    
    return static_cast<double>(m_compressedDataSize) / m_originalDataSize;
}

void AdaptiveSampler::reset()
{
    QMutexLocker locker(&m_mutex);
    
    // 重置所有状态
    m_lastDataPoints.clear();
    m_currentIntervals.clear();
    m_changeRates.clear();
    m_originalDataSize = 0;
    m_compressedDataSize = 0;
    
    qDebug() << "重置采样器状态";
}

bool AdaptiveSampler::shouldSample(const QString &metricName, double value, const QDateTime &timestamp)
{
    // 如果是第一个数据点，总是采样
    if (!m_lastDataPoints.contains(metricName)) {
        return true;
    }
    
    const QPair<QDateTime, double> &lastPoint = m_lastDataPoints[metricName];
    
    // 根据不同的采样策略决定是否采样
    switch (m_samplingStrategy) {
        case FixedRate: {
            // 固定采样率：检查是否达到采样间隔
            qint64 elapsed = lastPoint.first.msecsTo(timestamp);
            return elapsed >= m_baseSamplingInterval;
        }
        case AdaptiveRate: {
            // 自适应采样率：检查是否达到当前采样间隔
            qint64 elapsed = lastPoint.first.msecsTo(timestamp);
            int currentInterval = m_currentIntervals.value(metricName, m_baseSamplingInterval);
            return elapsed >= currentInterval;
        }
        case EventBased: {
            // 事件触发采样：这里简化为值变化超过阈值时采样
            double lastValue = lastPoint.second;
            double change = std::abs(value - lastValue) / (std::abs(lastValue) + 0.000001); // 避免除以零
            return change >= m_deltaThreshold;
        }
        case DeltaBased: {
            // 变化量触发采样：检查变化量是否超过阈值
            double lastValue = lastPoint.second;
            double absoluteChange = std::abs(value - lastValue);
            double relativeChange = absoluteChange / (std::abs(lastValue) + 0.000001); // 避免除以零
            
            // 如果相对变化超过阈值或者绝对变化很大，则采样
            return relativeChange >= m_deltaThreshold || absoluteChange >= 1.0;
        }
        default:
            return true;
    }
}

void AdaptiveSampler::updateAdaptiveInterval(const QString &metricName, double value)
{
    // 如果是第一个或第二个数据点，使用基础采样间隔
    if (!m_lastDataPoints.contains(metricName) || !m_changeRates.contains(metricName) || m_changeRates[metricName].isEmpty()) {
        m_currentIntervals[metricName] = m_baseSamplingInterval;
        return;
    }
    
    // 计算变化率
    double lastValue = m_lastDataPoints[metricName].second;
    double changeRate = std::abs(value - lastValue) / (std::abs(lastValue) + 0.000001); // 避免除以零
    
    // 保存变化率历史（最多保留10个）
    QVector<double> &rates = m_changeRates[metricName];
    rates.append(changeRate);
    if (rates.size() > 10) {
        rates.removeFirst();
    }
    
    // 计算平均变化率
    double avgChangeRate = 0.0;
    for (double rate : rates) {
        avgChangeRate += rate;
    }
    avgChangeRate /= rates.size();
    
    // 根据变化率调整采样间隔
    int newInterval;
    if (avgChangeRate >= m_deltaThreshold * 2) {
        // 变化剧烈，减小采样间隔
        newInterval = m_minSamplingInterval;
    } else if (avgChangeRate >= m_deltaThreshold) {
        // 变化明显，使用较小的采样间隔
        newInterval = qMax(m_minSamplingInterval, m_baseSamplingInterval / 2);
    } else if (avgChangeRate <= m_deltaThreshold / 4) {
        // 变化很小，增大采样间隔
        newInterval = qMin(m_maxSamplingInterval, m_baseSamplingInterval * 2);
    } else {
        // 变化适中，使用基础采样间隔
        newInterval = m_baseSamplingInterval;
    }
    
    // 如果间隔发生变化，发出信号
    if (m_currentIntervals.value(metricName, m_baseSamplingInterval) != newInterval) {
        m_currentIntervals[metricName] = newInterval;
        emit samplingIntervalChanged(metricName, newInterval);
    }
}

QVector<QPair<QDateTime, double>> AdaptiveSampler::runLengthEncode(const QVector<QPair<QDateTime, double>> &data)
{
    if (data.isEmpty()) {
        return QVector<QPair<QDateTime, double>>();
    }
    
    QVector<QPair<QDateTime, double>> compressed;
    
    // 游程编码：连续相同值只保留首尾
    double currentValue = data.first().second;
    QDateTime startTime = data.first().first;
    QDateTime endTime = startTime;
    
    for (int i = 1; i < data.size(); ++i) {
        const QPair<QDateTime, double> &point = data[i];
        
        // 如果值相同（允许微小误差），更新结束时间
        if (std::abs(point.second - currentValue) < 0.000001) {
            endTime = point.first;
        } else {
            // 值不同，保存当前游程并开始新游程
            compressed.append(qMakePair(startTime, currentValue));
            compressed.append(qMakePair(endTime, currentValue));
            
            currentValue = point.second;
            startTime = point.first;
            endTime = startTime;
        }
    }
    
    // 添加最后一个游程
    compressed.append(qMakePair(startTime, currentValue));
    compressed.append(qMakePair(endTime, currentValue));
    
    return compressed;
}

QVector<QPair<QDateTime, double>> AdaptiveSampler::runLengthDecode(const QVector<QPair<QDateTime, double>> &compressedData)
{
    if (compressedData.isEmpty() || compressedData.size() % 2 != 0) {
        return compressedData; // 如果数据为空或不是偶数个点（格式错误），直接返回
    }
    
    QVector<QPair<QDateTime, double>> decompressed;
    
    // 游程解码：根据首尾时间和值，生成中间点
    for (int i = 0; i < compressedData.size(); i += 2) {
        QDateTime startTime = compressedData[i].first;
        QDateTime endTime = compressedData[i + 1].first;
        double value = compressedData[i].second;
        
        // 添加起始点
        decompressed.append(qMakePair(startTime, value));
        
        // 如果起始时间和结束时间不同，添加中间点
        if (startTime != endTime) {
            // 计算中间点数量（假设原始采样间隔为1秒）
            qint64 seconds = startTime.secsTo(endTime);
            for (qint64 j = 1; j < seconds; ++j) {
                QDateTime middleTime = startTime.addSecs(j);
                decompressed.append(qMakePair(middleTime, value));
            }
            
            // 添加结束点
            decompressed.append(qMakePair(endTime, value));
        }
    }
    
    return decompressed;
}

QVector<QPair<QDateTime, double>> AdaptiveSampler::deltaEncode(const QVector<QPair<QDateTime, double>> &data)
{
    if (data.isEmpty()) {
        return QVector<QPair<QDateTime, double>>();
    }
    
    QVector<QPair<QDateTime, double>> compressed;
    
    // 增量编码：存储第一个点的绝对值，后续点存储与前一点的差值
    compressed.append(data.first());
    
    for (int i = 1; i < data.size(); ++i) {
        QDateTime timestamp = data[i].first;
        double delta = data[i].second - data[i - 1].second;
        compressed.append(qMakePair(timestamp, delta));
    }
    
    return compressed;
}

QVector<QPair<QDateTime, double>> AdaptiveSampler::deltaDecode(const QVector<QPair<QDateTime, double>> &compressedData)
{
    if (compressedData.isEmpty()) {
        return QVector<QPair<QDateTime, double>>();
    }
    
    QVector<QPair<QDateTime, double>> decompressed;
    
    // 增量解码：第一个点是绝对值，后续点通过累加差值恢复
    decompressed.append(compressedData.first());
    double currentValue = compressedData.first().second;
    
    for (int i = 1; i < compressedData.size(); ++i) {
        QDateTime timestamp = compressedData[i].first;
        currentValue += compressedData[i].second; // 累加差值
        decompressed.append(qMakePair(timestamp, currentValue));
    }
    
    return decompressed;
}

QVector<QPair<QDateTime, double>> AdaptiveSampler::piecewiseCompress(const QVector<QPair<QDateTime, double>> &data)
{
    if (data.size() <= 2) {
        return data; // 如果点数太少，不压缩
    }
    
    QVector<QPair<QDateTime, double>> compressed;
    
    // 分段线性压缩：保留关键点，使用直线近似中间点
    compressed.append(data.first()); // 添加第一个点
    
    int startIdx = 0;
    for (int i = 2; i < data.size(); ++i) {
        // 计算当前线段（从startIdx到i-1）的斜率
        double x1 = data[startIdx].first.toMSecsSinceEpoch();
        double y1 = data[startIdx].second;
        double x2 = data[i - 1].first.toMSecsSinceEpoch();
        double y2 = data[i - 1].second;
        
        double slope = (y2 - y1) / (x2 - x1 + 0.000001); // 避免除以零
        
        // 计算当前点到线段的垂直距离
        double x = data[i].first.toMSecsSinceEpoch();
        double y = data[i].second;
        double expectedY = y1 + slope * (x - x1);
        double distance = std::abs(y - expectedY);
        
        // 如果距离超过阈值，添加前一个点作为关键点，并开始新的线段
        if (distance > m_deltaThreshold * std::abs(y1)) {
            compressed.append(data[i - 1]);
            startIdx = i - 1;
        }
    }
    
    compressed.append(data.last()); // 添加最后一个点
    
    return compressed;
}

QVector<QPair<QDateTime, double>> AdaptiveSampler::piecewiseDecompress(const QVector<QPair<QDateTime, double>> &compressedData)
{
    if (compressedData.size() <= 1) {
        return compressedData;
    }
    
    QVector<QPair<QDateTime, double>> decompressed;
    
    // 分段线性解压：根据关键点之间的线性关系，插值生成中间点
    for (int i = 0; i < compressedData.size() - 1; ++i) {
        QDateTime startTime = compressedData[i].first;
        QDateTime endTime = compressedData[i + 1].first;
        double startValue = compressedData[i].second;
        double endValue = compressedData[i + 1].second;
        
        // 添加起始点
        decompressed.append(qMakePair(startTime, startValue));
        
        // 如果起始时间和结束时间不同，添加中间点
        if (startTime != endTime) {
            // 计算中间点数量（假设原始采样间隔为1秒）
            qint64 seconds = startTime.secsTo(endTime);
            double valueStep = (endValue - startValue) / seconds;
            
            for (qint64 j = 1; j < seconds; ++j) {
                QDateTime middleTime = startTime.addSecs(j);
                double middleValue = startValue + valueStep * j;
                decompressed.append(qMakePair(middleTime, middleValue));
            }
        }
    }
    
    // 添加最后一个点
    decompressed.append(compressedData.last());
    
    return decompressed;
}