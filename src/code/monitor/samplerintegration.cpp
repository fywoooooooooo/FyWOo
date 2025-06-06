// samplerintegration.cpp
#include "src/include/monitor/samplerintegration.h"
#include <QDebug>

SamplerIntegration::SamplerIntegration(QObject *parent)
    : QObject(parent)
    , m_threadedSampler(new ThreadedSampler(this))
    , m_anomalyDetector(new AnomalyDetector(this))
{
    // 连接多线程采样器的信号到异常检测器
    connect(m_threadedSampler, &ThreadedSampler::performanceDataUpdated,
            this, &SamplerIntegration::processPerformanceData);
    
    // 连接异常检测器的信号
    connect(m_anomalyDetector, &AnomalyDetector::anomalyDetected,
            this, &SamplerIntegration::onAnomalyDetected);
}

SamplerIntegration::~SamplerIntegration()
{
    stopMonitoring();
}

void SamplerIntegration::startHighPrecisionMonitoring()
{
    // 使用更高的采样频率启动多线程采样器
    // CPU: 200ms, 内存: 500ms, 磁盘: 500ms, 网络: 200ms
    m_threadedSampler->startSampling(200, 500, 500, 200);
}

void SamplerIntegration::stopMonitoring()
{
    m_threadedSampler->stopSampling();
}

void SamplerIntegration::setAnomalyThreshold(double threshold)
{
    m_anomalyDetector->setThresholdFactor(threshold);
}

void SamplerIntegration::setStorage(DataStorage *storage)
{
    m_threadedSampler->setStorage(storage);
}

void SamplerIntegration::setChartWidgets(ChartWidget *cpu, ChartWidget *mem, ChartWidget *gpu, ChartWidget *net)
{
    m_threadedSampler->setChartWidgets(cpu, mem, gpu, net);
}

void SamplerIntegration::processPerformanceData(double cpuUsage, double memoryUsage, double diskIO, double networkUsage)
{
    // 添加数据点到异常检测器
    m_anomalyDetector->addCpuDataPoint(cpuUsage);
    m_anomalyDetector->addMemoryDataPoint(memoryUsage);
    m_anomalyDetector->addDiskDataPoint(diskIO);
    m_anomalyDetector->addNetworkDataPoint(networkUsage);
    
    // 执行异常检测
    m_anomalyDetector->detectCpuAnomaly();
    m_anomalyDetector->detectMemoryAnomaly();
    m_anomalyDetector->detectDiskAnomaly();
    m_anomalyDetector->detectNetworkAnomaly();
}

void SamplerIntegration::onAnomalyDetected(const QString& source, double value, double threshold, const QDateTime& timestamp)
{
    // 异常检测结果
    
    // 这里可以添加通知用户的代码，例如系统托盘通知或弹窗
}